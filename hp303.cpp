/**
 * Title: HP303 - Resonant Highpass Filter
 * A resonant highpass filter with TB-303 style characteristics
 */
#include "usermodfx.h"
#include "osc_api.h"
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

    for (uint32_t frame = 0; frame < frames; frame++)
    {
        float x[2] = {main_xn[2 * frame], main_xn[2 * frame + 1]};
        float y[2];
        filter.process_frame(coeff, params, x, y);
        main_yn[2 * frame] = y[0];
        main_yn[2 * frame + 1] = y[1];
    }
}

float rescale_cutoff(float p_value)
{
    return fasterpowf(p_value, 0.7f); // get into midrange sooner
}

float rescale_resonance(float p_value)
{
    return fasterpowf(p_value, 1.f); // ease into max resonance very gently
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float valf = q31_to_f32(value); // 10 bit value rescaled to pvalue
    switch (index) {
    case k_user_modfx_param_time:
        s_param_cutoff = rescale_cutoff(valf);
        break;
    case 1:
        s_param_resonance = rescale_resonance(valf);
        break;
    default:
        break;
    }
}