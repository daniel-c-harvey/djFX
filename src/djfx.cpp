/**
 * Title: HP303 - Resonant Highpass Filter
 * A resonant highpass filter with TB-303 style characteristics
 */
#include "djfx.hpp"
#include "osc_api.h"

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    // uparams = UserParameters();

    // hp_butterworth_params = ButterworthParameters();
    // hp_compensated_params = CompensatedParameters();
    // hp_saturated_params = SaturatedParameters();

    // hp_butterworth = ButterworthHP<2, FilterParameters>(k_samplerate, &hp_butterworth_params);
    // hp_compensated = Compensated<2, FilterParameters>(&hp_butterworth, &hp_compensated_params);
    // hp_saturated = Saturated<2, FilterParameters>(&hp_compensated, &hp_saturated_params);

    // hp_filter = &hp_saturated;


    // lp_butterworth_params = ButterworthParameters();
    // lp_compensated_params = CompensatedParameters();
    // lp_saturated_params = SaturatedParameters();

    // lp_butterworth = ButterworthHP<2, FilterParameters>(k_samplerate, &lp_butterworth_params);
    // lp_compensated = Compensated<2, FilterParameters>(&lp_butterworth, &lp_compensated_params);
    // lp_saturated = Saturated<2, FilterParameters>(&lp_compensated, &lp_saturated_params);

    // lp_filter = &lp_saturated;
}

void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames)
{
    hp_filter->prepare_parameters(uparams.getHPParams());
    lp_filter->prepare_parameters(uparams.getLPParams());

    NormalCoefficients hp_coeff = hp_filter->prepare_coefficients();
    NormalCoefficients lp_coeff = lp_filter->prepare_coefficients();

    for (uint32_t frame = 0; frame < frames; frame++)
    {
        float x[2] = {main_xn[2 * frame], main_xn[2 * frame + 1]};
        float y[2];

        hp_filter->process_frame(hp_coeff, x, y);
        lp_filter->process_frame(lp_coeff, y, y);

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
        uparams.setLP(valf);
        break;
    default:
        break;
    }
}