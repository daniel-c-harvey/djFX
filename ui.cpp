#include "ui.hpp"
#include "osc_api.h"

/** CONSTANTS */

static const float a1 = 8.f;
static const float a2 = 0.8f;
static const float b1 = 96.f;
static const float b2 = 8.f;
static const float c1 = 0.08125f;
static const float c2 = 0.425f;
static const float z1 = 0.42f;
static const float z2 = 0.26f;

static const uint8_t k_crossbands = 2;
static const float bands[k_crossbands][4] = {
    {a1, b1, c1, z1},
    {a2, b2, c2, z2}
};

/** /CONSTANTS */

/// @brief Tunable logistic function (sigmoid)
/// @param a slope
/// @param b slope 2
/// @param c offset
/// @param z portion scalar
/// @return H(x)
static inline float HP_H(float x, float a, float b, float c, float z)
{
    return z * a / (a + fasterexpf(b * (c - x))) - 0.02f;
}

static float HP_resoH(float x)
{
    float acc = 0;

    for (uint8_t band = 0; band < k_crossbands; band++)
    {
        acc += HP_H(x, bands[band][0], bands[band][1], bands[band][2], bands[band][3]);
    }

    return acc;
}

void UserParameters::setHP(float p_value)
{
    p_hp_cutoff = fasterpowf(p_value, 0.4f) * 0.8;
    p_hp_resonance = HP_resoH(p_value) * 0.28f;
}

float UserParameters::getHPCutoff()
{
    return p_hp_cutoff;
}

float UserParameters::getHPResonance()
{
    return p_hp_resonance;
}