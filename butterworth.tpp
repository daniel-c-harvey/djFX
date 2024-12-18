#pragma once

#include "butterworth.hpp"

static const float Qbase = 1.f / sqrtf(2.f);

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TUIParams, typename TFilterParams>
inline Filter<k_channels, TFeedbackLine, TCoefficients, TUIParams, TFilterParams>::Filter(TFilterParams *p)
: params(p) {}

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TUIParams, typename TFilterParams>
FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TUIParams, TFilterParams>::FilterDecorator(FilterBase<k_channels, TFeedbackLine, TCoefficients, TUIParams> *f, TFilterParams *p)
: Filter<k_channels, TFeedbackLine, TCoefficients, TUIParams, TFilterParams>(p), filter_ptr(f) { }

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TUIParams, typename TFilterParams>
void FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TUIParams, TFilterParams>::prepare_parameters(const TUIParams &params)
{
    this->filter_ptr->prepare_parameters(params);
}

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TUIParams, typename TFilterParams>
TCoefficients FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TUIParams, TFilterParams>::prepare_coefficients()
{
    return filter_ptr->prepare_coefficients();
}

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TUIParams, typename TFilterParams>
void FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TUIParams, TFilterParams>::process_frame(const TCoefficients &coeff, const float x[k_channels], float y[k_channels])
{
    this->filter_ptr->process_frame(coeff, x, y);
}

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TUIParams, typename TFilterParams>
void FilterDecorator<k_channels, TFeedbackLine, TCoefficients, TUIParams, TFilterParams>::process_channel_frame(TFeedbackLine &state, const TCoefficients &coeff, const float &x, float &y)
{
    this->filter_ptr->process_channel_frame(state, coeff, x, y);
}

template <int k_channels, typename TUIParams>
Butterworth<k_channels, TUIParams>::Butterworth(const unsigned long& p_sample_rate, ButterworthParameters *p_params)
: Filter<k_channels, FeedbackLine, NormalCoefficients, TUIParams, ButterworthParameters>(p_params), sample_rate(p_sample_rate) {}

template <int k_channels, typename TUIParams>
void Butterworth<k_channels, TUIParams>::prepare_parameters(const TUIParams& params)
{
    // Exponential frequency scaling
    this->params->cutoff = 20.f * fasterpowf(1000.f, params.p_cutoff);//fasterpowf(10.f, params.p_cutoff * 4.30061f) + 15.f;

    // Resonance response
    this->params->res = params.p_resonance;

    // Base Q of 0.707 (Butterworth) plus resonance
    this->params->Q = Qbase + this->params->res * 10.f; 
}

template <int k_channels, typename TUIParams>
void Butterworth<k_channels, TUIParams>::process_frame(const NormalCoefficients& coeff, 
                                              const float x[k_channels], 
                                              float y[k_channels])
{
    for (uint16_t channel = 0; channel < k_channels; channel++)
    {
        process_channel_frame(state[channel], coeff, x[channel], y[channel]);
    }
}

template <int k_channels, typename TUIParams>
void Butterworth<k_channels, TUIParams>::process_channel_frame(FeedbackLine& state,
                                                               const NormalCoefficients& coeff,
                                                               const float& x, 
                                                               float& y)
{
    this->filter(state, coeff, x, y);
    
    // Update feedback state
    state.x[1] = state.x[0];
    state.x[0] = x;
    state.y[1] = state.y[0];
    state.y[0] = y;
    state.fb = y;
}

template <int k_channels, typename TUIParams>
ButterworthHP<k_channels, TUIParams>::ButterworthHP(const unsigned long& p_sample_rate, ButterworthParameters *p_params)
: Butterworth<k_channels, TUIParams>(p_sample_rate, p_params) {}

template <int k_channels, typename TUIParams>
NormalCoefficients ButterworthHP<k_channels, TUIParams>::prepare_coefficients()
{
    // Convert parameters to filter coefficients with prewarping
    const float w0 = osc_tanpif(this->params->cutoff / this->sample_rate);
    const float cosw0 = (1.0f - w0 * w0) / (1.0f + w0 * w0);
    const float alpha = w0 / (1.0f + w0 * w0) / this->params->Q;
    
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

template <int k_channels, typename TUIParams>
void ButterworthHP<k_channels, TUIParams>::filter(FeedbackLine& state, const NormalCoefficients &coeff, const float &x, float &y)
{
    // Filter
    y = coeff.b0 * x + coeff.b1 * state.x[0] + coeff.b2 * state.x[1]
                - coeff.a1 * state.y[0] - coeff.a2 * state.y[1];
}

template <int k_channels, typename TUIParams>
void Compensated<k_channels, TUIParams>::prepare_parameters(const TUIParams& params)
{
    FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, TUIParams, CompensatedParameters>::prepare_parameters(params);

    // Reduced feedback with compensation for volume loss
    this->params->fb_amount = this->params->res * 0.24f;
    
    // Volume compensation increases with resonance
    this->params->vol_comp = 1.f + (this->params->fb_amount);
}    

template <int k_channels, typename TUIParams>
void Compensated<k_channels, TUIParams>::process_channel_frame(FeedbackLine &state, 
                                                               const NormalCoefficients &coeff,
                                                               const float& x, 
                                                               float& y)
{
    FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, TUIParams, CompensatedParameters>::process_channel_frame(state, coeff, x, y);
    y *= this->params->vol_comp;
}

template <int k_channels, typename TUIParams>
void Saturated<k_channels, TUIParams>::prepare_parameters(const TUIParams& params)
{
    FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, TUIParams, SaturatedParameters>::prepare_parameters(params);

   // drive based on resonance
    this->params->drive = 1.f + this->params->res * 10.f;
}

template <int k_channels, typename TUIParams>
void Saturated<k_channels, TUIParams>::process_channel_frame(FeedbackLine &state, const NormalCoefficients &coeff, const float &x, float &y)
{
 // Process each channel with resonance feedback
    float input = x - this->params->fb_amount * tb303_tanh(state.fb * 0.9f);
    
    FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, TUIParams, SaturatedParameters>::process_channel_frame(state, coeff, input, y);

    // Apply saturation with drive that increases less with resonance
    y = tb303_tanh(y * this->params->drive) * (1.f / this->params->drive);
}