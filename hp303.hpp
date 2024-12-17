// hp303.hpp
#pragma once

#include "usermodfx.h"
#include "butterworth.hpp"
#include "ui.hpp"
#include "stdint.h"

static UserParameters uparams;

static ButterworthParameters butterworth_params = ButterworthParameters();
static CompensatedParameters compensated_params = CompensatedParameters();

static ButterworthHP<2> butterworth = ButterworthHP<2>(k_samplerate, &butterworth_params);
static Compensated<2> compensated = Compensated<2>(&butterworth, &compensated_params);
static FilterBase<2, FeedbackLine, NormalCoefficients>* hp_filter = &compensated;

void MODFX_INIT(uint32_t platform, uint32_t api);
void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames);
void MODFX_PARAM(uint8_t index, int32_t value);

