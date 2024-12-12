/**
 * Title: HP303 - Resonant Highpass Filter
 * A resonant highpass filter with TB-303 style characteristics
 */
#include "usermodfx.h"
#include "osc_api.h"
#include "float_math.h"

static float s_param_cutoff;
static float s_param_resonance;

typedef struct {
    float x1;  // Previous input
    float x2;  // Input from 2 samples ago
    float y1;  // Previous output
    float y2;  // Output from 2 samples ago
    float fb;  // Feedback value for resonance
} FilterState;

static FilterState s_state_l, s_state_r;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
    s_param_cutoff = 0.f;
    s_param_resonance = 0.f;
    s_state_l = (FilterState){0.f, 0.f, 0.f, 0.f, 0.f};
    s_state_r = (FilterState){0.f, 0.f, 0.f, 0.f, 0.f};
}

// TB-303 style tanh approximation for feedback saturation
static inline float tb303_tanh(float x) {
    // Soft clip approximation
    if (x > 3.f) return 1.f;
    if (x < -3.f) return -1.f;
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames)
{
    // Convert parameters to filter coefficients
    const float cutoff = 20.f + s_param_cutoff * 9980.f;  // 20Hz to 10kHz
    const float w0 = cutoff / k_samplerate;
    const float cosw0 = osc_cosf(w0);
    
    // Modified resonance response
    const float res = s_param_resonance * 0.67f;  // Limit maximum resonance
    const float Q = 0.5f + fastexpf(res * 1.2f);  // Gentler exponential curve
    const float alpha = osc_sinf(w0)/(2.f * Q);
    
    // Calculate filter coefficients (highpass)
    const float a0 = 1.f + alpha;
    const float a1 = -2.f * cosw0;
    const float a2 = 1.f - alpha;
    const float b0 = (1.f + cosw0) / 2.f;
    const float b1 = -(1.f + cosw0);
    const float b2 = (1.f + cosw0) / 2.f;
    
    // Normalize coefficients
    const float b0_n = b0 / a0;
    const float b1_n = b1 / a0;
    const float b2_n = b2 / a0;
    const float a1_n = a1 / a0;
    const float a2_n = a2 / a0;
    
    // Reduced feedback with compensation for volume loss
    const float fb_amount = res * 1.8f;  // Reduced feedback
    
    // Volume compensation increases with resonance
    const float vol_comp = 1.f + (res * 2.f);  // Compensate for resonance-induced volume loss
    
    const float *in_ptr = main_xn;
    float *out_ptr = main_yn;
    
    for (uint32_t frame = 0; frame < frames; frame++) {
        // Process left channel with resonance feedback
        float input_l = in_ptr[0] - fb_amount * tb303_tanh(s_state_l.fb * 0.6f);  // Reduced feedback saturation
        float yn_l = b0_n * input_l + b1_n * s_state_l.x1 + b2_n * s_state_l.x2
                    - a1_n * s_state_l.y1 - a2_n * s_state_l.y2;
        
        // Update left state
        s_state_l.x2 = s_state_l.x1;
        s_state_l.x1 = input_l;
        s_state_l.y2 = s_state_l.y1;
        s_state_l.y1 = yn_l;
        s_state_l.fb = yn_l;
        
        // Process right channel with resonance feedback
        float input_r = in_ptr[1] - fb_amount * tb303_tanh(s_state_r.fb * 0.6f);
        float yn_r = b0_n * input_r + b1_n * s_state_r.x1 + b2_n * s_state_r.x2
                    - a1_n * s_state_r.y1 - a2_n * s_state_r.y2;
        
        // Update right state
        s_state_r.x2 = s_state_r.x1;
        s_state_r.x1 = input_r;
        s_state_r.y2 = s_state_r.y1;
        s_state_r.y1 = yn_r;
        s_state_r.fb = yn_r;
        
        // Apply saturation with drive that increases less with resonance
        const float drive = 1.f + res * 1.2f;  // Reduced drive scaling
        yn_l = tb303_tanh(yn_l * drive) * (1.f / drive);
        yn_r = tb303_tanh(yn_r * drive) * (1.f / drive);
        
        // Apply volume compensation
        yn_l *= vol_comp;
        yn_r *= vol_comp;
        
        // Write outputs
        out_ptr[0] = yn_l;
        out_ptr[1] = yn_r;
        
        // Advance pointers
        in_ptr += 2;
        out_ptr += 2;
    }
    
    // Handle sub buffer if present
    if (sub_yn != NULL) {
        for (uint32_t i = 0; i < frames * 2; i++) {
            sub_yn[i] = main_yn[i];
        }
    }
}

void MODFX_PARAM(uint8_t index, int32_t value)
{
    const float valf = q31_to_f32(value);
    switch (index) {
    case 0:
        s_param_cutoff = valf;
        break;
    case 1:
        s_param_resonance = valf;
        break;
    default:
        break;
    }
}