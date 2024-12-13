/**
 * Title: HP303 - Resonant Highpass Filter
 * A resonant highpass filter with TB-303 style characteristics
 */
#include "usermodfx.h"
#include "butterworth.hpp"

static float s_param_cutoff;
static float s_param_resonance;
static Butterworth<2> filter;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    s_param_cutoff = 0.f;
    s_param_resonance = 0.f;
    filter.init(k_samplerate);
}

void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames)
{
    NormalCoefficients coeff = filter.process_block(s_param_cutoff, s_param_resonance);
    
    // Reduced feedback with compensation for volume loss
    const float fb_amount = s_param_resonance * 1.67f * 1.8f;
    
    // Volume compensation increases with resonance
    const float vol_comp = 1.f + (fb_amount);  // Compensate for resonance-induced volume loss
    
    const float *in_ptr = main_xn;
    float *out_ptr = main_yn;
    
    for (uint32_t frame = 0; frame < frames; frame++)
    {        
        filter.process_frame(coeff, in_ptr, out_ptr);
        
        // Apply saturation with drive that increases less with resonance
        const float drive = 1.f + s_param_resonance * 1.67f * 1.2f;  // Reduced drive scaling
        out_ptr[0] = tb303_tanh(out_ptr[0] * drive) * (1.f / drive);
        out_ptr[1] = tb303_tanh(out_ptr[1] * drive) * (1.f / drive);
        
        // Apply volume compensation
        out_ptr[0] *= vol_comp;
        out_ptr[1] *= vol_comp;
        
        // Advance pointers
        in_ptr += 2;
        out_ptr += 2;
    }
}

float rescale_cutoff(float p_value)
{
    return fasterpowf(p_value, 4.0f);
}

float rescale_resonance(float p_value)
{
    return fasterpowf(p_value, 1.2f);
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float valf = q31_to_f32(value);
    switch (index) {
    case 0:
        s_param_cutoff = rescale_cutoff(valf);
        break;
    case 1:
        s_param_resonance = rescale_resonance(valf);
        break;
    default:
        break;
    }
}