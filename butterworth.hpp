#pragma once

#include "util.hpp"

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
class Filter
{
    public:
        /// @brief Initialize the filter accross all channels
        /// @param sample_rate 
        virtual void init(const unsigned long& sample_rate) = 0;
        
        /// @brief Prepare the filter channels to process all frames in this block
        /// @param param_cutoff [0, 1]
        /// @param param_resonance [0, 1]
        virtual TParams prepare_parameters(const float param_cutoff, const float param_resonance) = 0;
        
        /// @brief Prepare the filter channels to process all frames in this block
        virtual TCoefficients prepare_coefficients(const TParams& params) = 0;
        
        /// @brief process the current frame samples for all channels
        /// @param x inputs samples
        /// @param y output samples
        virtual void process_frame(const TCoefficients& coeff, const TParams& params, 
                                   const float x[k_channels], float y[k_channels]) = 0;
        
    protected:
        /// @brief process the current frame sample for given channel
        /// @param x inputs sample
        /// @param y output sample
        virtual void process_channel_frame(TFeedbackLine& state, const TCoefficients& coeff, 
                                           const TParams& params, const float& x, float& y) = 0;
};

struct Parameters 
{
    float cutoff;
    float res;
    float Q;
};

typedef struct {
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
} NormalCoefficients;

typedef struct {
    float x[2];  // Previous inputs
    float y[2];  // Previous outputs
    float fb;  // Feedback value for resonance
} FeedbackLine;

template <int k_channels, typename TParams>
class __Butterworth : Filter<k_channels, FeedbackLine, NormalCoefficients, TParams>
{
    public:
        virtual void init(const unsigned long& sample_rate);
        
        virtual TParams prepare_parameters(const float param_cutoff, 
                                           const float param_resonance) override;
                
        virtual NormalCoefficients prepare_coefficients(const TParams& params) override;
        
        virtual void process_frame(const NormalCoefficients& coeff, 
                                   const TParams& params, 
                                   const float x[k_channels], 
                                   float y[k_channels]) override;
    protected:
        uint32_t sample_rate;
        FeedbackLine state[k_channels];

        virtual void process_channel_frame(FeedbackLine& state, 
                                           const NormalCoefficients& coeff, 
                                           const TParams& params, 
                                           const float& x, 
                                           float& y) override;
};

template <int k_channels, typename TParams>
void __Butterworth<k_channels, TParams>::init(const unsigned long& sample_rate)
{
    this->sample_rate = sample_rate;
}

template <int k_channels, typename TParams>
TParams __Butterworth<k_channels, TParams>::prepare_parameters(const float param_cutoff,
                                                               const float param_resonance)
{
    TParams params;
    
    // Exponential frequency scaling
    params.cutoff = fasterpowf(10.f, param_cutoff * 4.30061f) + 19; // scales exponentially to around 10kHz

    // Resonance response
    params.res = param_resonance;
    params.Q = 0.707f + params.res * 10.f; // Base Q of 0.707 (Butterworth) plus resonance

    return params;
}

template <int k_channels, typename TParams>
NormalCoefficients __Butterworth<k_channels, TParams>::prepare_coefficients(const TParams& params)
{
    // Convert parameters to filter coefficients
    const float w0 = params.cutoff / k_samplerate;
    const float cosw0 = osc_cosf(w0);
    
    const float alpha = osc_sinf(w0)/(2.f * params.Q);
    
    // Calculate filter coefficients (highpass)
    const float a0 = 1.f + alpha;
    const float a1 = -2.f * cosw0;
    const float a2 = 1.f - alpha;
    const float b0 = (1.f + cosw0) / 2.f;
    const float b1 = -(1.f + cosw0);
    const float b2 = (1.f + cosw0) / 2.f;
    
    NormalCoefficients coeff = {
        .a1 = a1 / a0,
        .a2 = a2 / a0,
        .b0 = b0 / a0,
        .b1 = b1 / a0,
        .b2 = b2 / a0
    };

    return coeff;
}

template <int k_channels, typename TParams>
void __Butterworth<k_channels, TParams>::process_frame(const NormalCoefficients& coeff, 
                                                       const TParams& params, 
                                                       const float x[k_channels], 
                                                       float y[k_channels])
{
    for (uint16_t channel = 0; channel < k_channels; channel++)
    {
        process_channel_frame(state[channel], coeff, params, x[channel], y[channel]);
    }
}

template <int k_channels, typename TParams>
void __Butterworth<k_channels, TParams>::process_channel_frame(FeedbackLine& state, 
                                                               const NormalCoefficients& coeff,
                                                               const TParams& params, 
                                                               const float& x, 
                                                               float& y)
{
    float input = x;

    // Filter
    float yn = coeff.b0 * input + coeff.b1 * state.x[0] + coeff.b2 * state.x[1]
                - coeff.a1 * state.y[0] - coeff.a2 * state.y[1];
    
    // Update feedback state
    state.x[1] = state.x[0];
    state.x[0] = input;
    state.y[1] = state.y[0];
    state.y[0] = yn;
    state.fb = yn;

    y = yn;
}

template <int k_channels>
class Butterworth : public __Butterworth<k_channels, Parameters> {};

struct CompensatedParameters : Parameters
{
    float vol_comp; // Compensate for resonance-induced volume loss
    float fb_amount; // feedback based on resonance
};

template <int k_channels, typename TParams>
class __CompensatedButterworth : public __Butterworth<k_channels, TParams> 
{
    public:
        virtual TParams prepare_parameters(const float param_cutoff, 
                                           const float param_resonance) override;
    protected:
        virtual void process_channel_frame(FeedbackLine& state, 
                                           const NormalCoefficients& coeff, 
                                           const TParams& params, 
                                           const float& x, 
                                           float& y) override;
};

template <int k_channels, typename TParams>
TParams __CompensatedButterworth<k_channels, TParams>::prepare_parameters(const float param_cutoff,
                                                                          const float param_resonance)
{
    TParams params = __Butterworth<k_channels, TParams>::prepare_parameters(param_cutoff, param_resonance);

    // Reduced feedback with compensation for volume loss
    params.fb_amount = params.res * 0.17f;
    
    // Volume compensation increases with resonance
    params.vol_comp = 1.f + (params.fb_amount); 

    return params;
}

template <int k_channels, typename TParams>
void __CompensatedButterworth<k_channels, TParams>::process_channel_frame(FeedbackLine &state, 
                                                                          const NormalCoefficients &coeff, 
                                                                          const TParams &params, 
                                                                          const float& x, 
                                                                          float& y)
{
    __Butterworth<k_channels, TParams>::process_channel_frame(state, coeff, params, x, y);
    y *= params.vol_comp;
}

template <int k_channels>
class CompensatedButterworth : public __CompensatedButterworth<k_channels, CompensatedParameters> {};

struct SaturatedParameters : CompensatedParameters
{
    float drive;
};

template <int k_channels, typename TParams>
class __SaturatedButterworth : public __CompensatedButterworth<k_channels, TParams>
{
    public:
        virtual TParams prepare_parameters(const float param_cutoff, 
                                           const float param_resonance) override;
    protected:
        void process_channel_frame(FeedbackLine& state, 
                                   const NormalCoefficients& coeff, 
                                   const TParams& params, 
                                   const float& x, 
                                   float& y) override;
};

template <int k_channels, typename TParams>
TParams __SaturatedButterworth<k_channels, TParams>::prepare_parameters(const float param_cutoff,
                                                                        const float param_resonance)
{
    TParams params = __CompensatedButterworth<k_channels, TParams>::prepare_parameters(param_cutoff, param_resonance);

   // drive based on resonance
    params.drive = 1.f;// + params.res * 1.1f;

    return params;
}

template <int k_channels, typename TParams>
void __SaturatedButterworth<k_channels, TParams>::process_channel_frame(FeedbackLine &state, 
                                                                      const NormalCoefficients &coeff, 
                                                                      const TParams &params, 
                                                                      const float& x, 
                                                                      float& y)
{
    // Process each channel with resonance feedback
    float input = x - params.fb_amount * tb303_tanh(state.fb * 0.9f);
    
    __CompensatedButterworth<k_channels, TParams>::process_channel_frame(state, coeff, params, input, y);

    // Apply saturation with drive that increases less with resonance
    y = tb303_tanh(y * params.drive) * (1.f / params.drive);
}

template <int k_channels>
class SaturatedButterworth : public __SaturatedButterworth<k_channels, SaturatedParameters> {};
