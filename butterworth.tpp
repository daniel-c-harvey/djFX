#pragma once

#include "butterworth.hpp"

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
inline Filter<k_channels, TFeedbackLine, TCoefficients, TParams>::Filter(TParams *p)
: params(p) {}

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TParams>::FilterDecorator(FilterBase<k_channels, TFeedbackLine, TCoefficients> *f, TParams *p)
: Filter<k_channels, TFeedbackLine, TCoefficients, TParams>(p), filter_ptr(f) { }

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
TCoefficients FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TParams>::prepare_coefficients()
{
    return filter_ptr->prepare_coefficients();
}

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
void FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TParams>::process_frame(const TCoefficients &coeff, const float x[k_channels], float y[k_channels])
{
    this->filter_ptr->process_frame(coeff, x, y);
}

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
void FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TParams>::process_channel_frame(TFeedbackLine &state, const TCoefficients &coeff, const float &x, float &y)
{
    this->filter_ptr->process_channel_frame(state, coeff, x, y);
}

template <int k_channels>
ButterworthHP<k_channels>::ButterworthHP(const unsigned long& p_sample_rate, ButterworthParameters *p_params)
: Filter<k_channels, FeedbackLine, NormalCoefficients, ButterworthParameters>(p_params), sample_rate(p_sample_rate) {}

// template <int k_channels>
// ButterworthParameters ButterworthHP<k_channels>::prepare_parameters(const float param_cutoff,
//                                                                     const float param_resonance)
// {
    
//     // return params;
// }

template <int k_channels>
NormalCoefficients ButterworthHP<k_channels>::prepare_coefficients()
{
    // Convert parameters to filter coefficients
    const float w0 = this->params->cutoff / sample_rate;
    const float cosw0 = osc_cosf(w0);
    
    const float alpha = osc_sinf(w0)/(2.f * this->params->Q);
    
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

template <int k_channels>
void ButterworthHP<k_channels>::process_frame(const NormalCoefficients& coeff, 
                                              const float x[k_channels], 
                                              float y[k_channels])
{
    // Exponential frequency scaling
    this->params->cutoff = fasterpowf(10.f, this->params->cutoff * 4.30061f) + 19;

    // Resonance response
    this->params->res = this->params->res;

    // Base Q of 0.707 (Butterworth) plus resonance
    this->params->Q = 0.707f + this->params->res * 10.f; 

    for (uint16_t channel = 0; channel < k_channels; channel++)
    {
        process_channel_frame(state[channel], coeff, x[channel], y[channel]);
    }
}

template <int k_channels>
void ButterworthHP<k_channels>::process_channel_frame(FeedbackLine& state, 
                                                      const NormalCoefficients& coeff,
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

// template <int k_channels>
// CompensatedParameters Compensated<k_channels>::prepare_parameters(const float param_cutoff,
//                                                                   const float param_resonance)
// {
//     CompensatedParameters params = FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, CompensatedParameters>::prepare_parameters(param_cutoff, param_resonance);

    

//     return params;
// }
template <int k_channels>
void Compensated<k_channels>::process_frame(const NormalCoefficients& coeff, const float x[k_channels], float y[k_channels])
{
    // Reduced feedback with compensation for volume loss
    this->params->fb_amount = this->params->res * 0.17f;
    
    // Volume compensation increases with resonance
    this->params->vol_comp = 1.f + (this->params->fb_amount); 

    FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, CompensatedParameters>::process_frame(coeff, x, y);
}
    

template <int k_channels>
void Compensated<k_channels>::process_channel_frame(FeedbackLine &state, 
                                                    const NormalCoefficients &coeff,
                                                    const float& x, 
                                                    float& y)
{
    FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, CompensatedParameters>::process_channel_frame(state, coeff, x, y);
    y *= Filter<k_channels, FeedbackLine, NormalCoefficients, CompensatedParameters>::params->vol_comp;
}

// template <int k_channels>
// SaturatedParameters Saturated<k_channels>::prepare_parameters(const float param_cutoff,
//                                                               const float param_resonance)
// {
//     SaturatedParameters params = FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, SaturatedParameters> ::prepare_parameters(param_cutoff, param_resonance);

//    // drive based on resonance
//     params.drive = 1.f;// + params.res * 1.1f;

//     return params;
// }

// template <int k_channels>
// void Saturated<k_channels>::process_channel_frame(FeedbackLine &state, 
//                                                   const NormalCoefficients &coeff, 
//                                                   const SaturatedParameters &params, 
//                                                   const float& x, 
//                                                   float& y)
// {
//     // Process each channel with resonance feedback
//     float input = x - params.fb_amount * tb303_tanh(state.fb * 0.9f);
    
//     FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, SaturatedParameters>::process_channel_frame(state, coeff, params, input, y);

//     // Apply saturation with drive that increases less with resonance
//     y = tb303_tanh(y * params.drive) * (1.f / params.drive);
// }