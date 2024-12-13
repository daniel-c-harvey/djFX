/**
 * Title: HP303 - Resonant Highpass Filter
 * A resonant highpass filter with TB-303 style characteristics
 */
#include "usermodfx.h"
#include "butterworth.hpp"

static float s_param_cutoff;
static float s_param_resonance;
static SaturatedButterworth<2> filter;

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
    SaturatedParameters params = filter.prepare_parameters(s_param_cutoff, s_param_resonance);
    NormalCoefficients coeff = filter.prepare_coefficients(params);    

    const float *in_ptr = main_xn;
    float *out_ptr = main_yn;
    
    for (uint32_t frame = 0; frame < frames; frame++)
    {        
        filter.process_frame(coeff, params, in_ptr, out_ptr);
                
        // Advance pointers
        in_ptr += 2;
        out_ptr += 2;
    }
}

float rescale_cutoff(float p_value)
{
    return fasterpowf(p_value, 11.0f);
}

float rescale_resonance(float p_value)
{
    return fasterpow2f(p_value);
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