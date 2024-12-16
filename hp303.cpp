/**
 * Title: HP303 - Resonant Highpass Filter
 * A resonant highpass filter with TB-303 style characteristics
 */
#include "hp303.hpp"
#include "osc_api.h"

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    uparams = UserParameters(); 
    filter = std::unique_ptr<Filter<2, FeedbackLine, NormalCoefficients, SaturatedParameters>>(
                new Saturated<2>( 
                    // std::unique_ptr<Filter<2, FeedbackLine, NormalCoefficients, SaturatedParameters>>(
                    //     new Compensated<2>(
                            std::unique_ptr<Filter<2, FeedbackLine, NormalCoefficients, SaturatedParameters>>(
                                new ButterworthHP<2, SaturatedParameters>(k_samplerate)
                            )
                        )
                    
            );
}

void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames)
{
    SaturatedParameters params = filter->prepare_parameters(uparams.getHPCutoff(), uparams.getHPResonance());
    NormalCoefficients coeff = filter->prepare_coefficients(params);

    for (uint32_t frame = 0; frame < frames; frame++)
    {
        float x[2] = {main_xn[2 * frame], main_xn[2 * frame + 1]};
        float y[2];
        filter->process_frame(coeff, params, x, y);
        main_yn[2 * frame] = y[0];
        main_yn[2 * frame + 1] = y[1];
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float valf = q31_to_f32(value); // 10 bit value rescaled to pvalue
    switch (index) {
    case k_user_modfx_param_time:
        uparams.setHP(valf);
        break;
    case k_user_modfx_param_depth:
        break;
    default:
        break;
    }
}