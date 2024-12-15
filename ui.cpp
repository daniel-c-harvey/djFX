#include "ui.hpp"
#include "osc_api.h"

/** CONSTANTS */

static const float a1 = 2.f;
static const float a2 = 0.8f;
static const float b1 = 72.f;
static const float b2 = 12.f;
static const float c1 = 0.08125f;
static const float c2 = 0.425f;
static const float z1 = 0.4f;
static const float z2 = 1.6f;

static const uint8_t k_crossbands = 2;
static const float bands[k_crossbands][4] = {
    {a1, b1, c1, z1},
    {a2, b2, c2, z2}
};

/** /CONSTANTS */


static inline float H(float x, float a, float b, float c, float z)
{
    return z * a / (k_crossbands * a + fasterexpf(b * (c - x))) - 0.02f;
}

static float resoH(float x)
{
    float acc = 0;

    for (uint8_t band = 0; band < k_crossbands; band++)
    {
        acc += H(x, bands[band][0], bands[band][1], bands[band][2], bands[band][3]);
    }

    return acc;
}

void UserParameters::setHP(float p_value)
{
    p_hp_cutoff = fasterpowf(p_value, 0.5f) * 0.8;
    p_hp_resonance = resoH(p_value) * 0.32f;
}

float UserParameters::getHPCutoff()
{
    return p_hp_cutoff;
}

float UserParameters::getHPResonance()
{
    return p_hp_resonance;
}