// hp303.hpp
#pragma once

#include "usermodfx.h"
#include "butterworth.hpp"
#include "ui.hpp"
#include "stdint.h"

static UserParameters uparams;

static ButterworthParameters hp_butterworth_params = ButterworthParameters();
static CompensatedParameters hp_compensated_params = CompensatedParameters();
static SaturatedParameters hp_saturated_params = SaturatedParameters();

static ButterworthHP<2, FilterParameters> hp_butterworth = ButterworthHP<2, FilterParameters>(k_samplerate, &hp_butterworth_params);
static Compensated<2, FilterParameters> hp_compensated = Compensated<2, FilterParameters>(&hp_butterworth, &hp_compensated_params);
static Saturated<2, FilterParameters> hp_saturated = Saturated<2, FilterParameters>(&hp_compensated, &hp_saturated_params);
static FilterBase<2, FeedbackLine, NormalCoefficients, FilterParameters>* hp_filter = &hp_saturated;

static ButterworthParameters lp_butterworth_params = ButterworthParameters();
static CompensatedParameters lp_compensated_params = CompensatedParameters();
static SaturatedParameters lp_saturated_params = SaturatedParameters();

static ButterworthLP<2, FilterParameters> lp_butterworth = ButterworthLP<2, FilterParameters>(k_samplerate, &lp_butterworth_params);
static Compensated<2, FilterParameters> lp_compensated = Compensated<2, FilterParameters>(&lp_butterworth, &lp_compensated_params);
static Saturated<2, FilterParameters> lp_saturated = Saturated<2, FilterParameters>(&lp_compensated, &lp_saturated_params);
static FilterBase<2, FeedbackLine, NormalCoefficients, FilterParameters>* lp_filter = &lp_saturated;

void MODFX_INIT(uint32_t platform, uint32_t api);
void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames);
void MODFX_PARAM(uint8_t index, int32_t value);

