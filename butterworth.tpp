#pragma once

#include "butterworth.hpp"

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TParams>::FilterDecorator(std::unique_ptr<Filter<k_channels, TFeedbackLine, TCoefficients, TParams>> f)
: filter_ptr(std::move(f))
{
    // :)
}

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
inline TCoefficients FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TParams>::prepare_coefficients(const TParams &params)
{
    return filter_ptr->prepare_coefficients(params);
}


template <int k_channels, typename TParams>
ButterworthHP<k_channels, TParams>::ButterworthHP(const unsigned long& sample_rate)
{
    this->sample_rate = sample_rate;
}

template <int k_channels, typename TParams>
TParams ButterworthHP<k_channels, TParams>::prepare_parameters(const float param_cutoff,
                                                               const float param_resonance)
{
    TParams params;
    
    // Exponential frequency scaling
    params.cutoff = fasterpowf(10.f, param_cutoff * 4.30061f) + 19;

    // Resonance response
    params.res = param_resonance;
    params.Q = 0.707f + params.res * 10.f; // Base Q of 0.707 (Butterworth) plus resonance

    return params;
}

template <int k_channels, typename TParams>
NormalCoefficients ButterworthHP<k_channels, TParams>::prepare_coefficients(const TParams& params)
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
void ButterworthHP<k_channels, TParams>::process_frame(const NormalCoefficients& coeff, 
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
void ButterworthHP<k_channels, TParams>::process_channel_frame(FeedbackLine& state, 
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
CompensatedParameters Compensated<k_channels>::prepare_parameters(const float param_cutoff,
                                                                  const float param_resonance)
{
    CompensatedParameters params = FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, CompensatedParameters>::prepare_parameters(param_cutoff, param_resonance);

    // Reduced feedback with compensation for volume loss
    params.fb_amount = params.res * 0.17f;
    
    // Volume compensation increases with resonance
    params.vol_comp = 1.f + (params.fb_amount); 

    return params;
}

template <int k_channels>
void Compensated<k_channels>::process_channel_frame(FeedbackLine &state, 
                                                    const NormalCoefficients &coeff, 
                                                    const CompensatedParameters &params, 
                                                    const float& x, 
                                                    float& y)
{
    FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, CompensatedParameters> ::process_channel_frame(state, coeff, params, x, y);
    y *= params.vol_comp;
}

template <int k_channels>
SaturatedParameters Saturated<k_channels>::prepare_parameters(const float param_cutoff,
                                                              const float param_resonance)
{
    SaturatedParameters params = FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, SaturatedParameters> ::prepare_parameters(param_cutoff, param_resonance);

   // drive based on resonance
    params.drive = 1.f;// + params.res * 1.1f;

    return params;
}

template <int k_channels>
void Saturated<k_channels>::process_channel_frame(FeedbackLine &state, 
                                                  const NormalCoefficients &coeff, 
                                                  const SaturatedParameters &params, 
                                                  const float& x, 
                                                  float& y)
{
    // Process each channel with resonance feedback
    float input = x - params.fb_amount * tb303_tanh(state.fb * 0.9f);
    
    FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, SaturatedParameters>::process_channel_frame(state, coeff, params, input, y);

    // Apply saturation with drive that increases less with resonance
    y = tb303_tanh(y * params.drive) * (1.f / params.drive);
}