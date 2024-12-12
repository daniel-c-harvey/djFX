#include "util.hpp"

static inline float tb303_tanh(float x)
 {
    // Soft clip approximation
    if (x > 3.f) return 1.f;
    if (x < -3.f) return -1.f;
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}