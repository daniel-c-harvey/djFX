#include "osc_api.h"
#include "util.hpp"

typedef struct {
    float x[2];  // Previous inputs
    float y[2];  // Previous outputs
    float fb;  // Feedback value for resonance
} FeedbackLine;

typedef struct {
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
} NormalCoefficients;

template <int kChannels>
class Butterworth
{
    public:
        /// @brief Initialize the filter accross all channels
        /// @param sample_rate 
        void init(const unsigned long& sample_rate);
        
        /// @brief Prepare the filter channels to process all frames in this block
        /// @param param_cutoff [0, 1]
        /// @param param_resonance [0, 1]
        NormalCoefficients process_block(const float param_cutoff, const float param_resonance);
        
        /// @brief process the current frame samples for all channels
        /// @param x inputs samples
        /// @param y output samples
        void process_frame(const NormalCoefficients& coeff, const float x[kChannels], float y[kChannels]);
        
    protected:
        uint32_t sample_rate;
        FeedbackLine state[kChannels];
};

template <int kChannels>
void Butterworth<kChannels>::init(const unsigned long& sample_rate)
{
    this->sample_rate = sample_rate;
}

template <int kChannels>
NormalCoefficients Butterworth<kChannels>::process_block(const float param_cutoff, const float param_resonance)
{
    // Convert parameters to filter coefficients
    const float cutoff = 20.f + param_cutoff * 9980.f;  // 20Hz to 10kHz
    const float w0 = cutoff / k_samplerate;
    const float cosw0 = osc_cosf(w0);
    
    // Modified resonance response
    const float res = param_resonance * 0.67f;  // Limit maximum resonance
    const float Q = 0.5f + fastexpf(res * 1.2f);  // Gentler exponential curve
    const float alpha = osc_sinf(w0)/(2.f * Q);
    
    // Calculate filter coefficients (highpass)
    const float a0 = 1.f + alpha;
    const float a1 = -2.f * cosw0;
    const float a2 = 1.f - alpha;
    const float b0 = (1.f + cosw0) / 2.f;
    const float b1 = -(1.f + cosw0);
    const float b2 = (1.f + cosw0) / 2.f;
    
    NormalCoefficients coeff = {
        .a1 = a1 / a0,
        .a2 = a2 / a0,
        .b0 = b0 / a0,
        .b1 = b1 / a0,
        .b2 = b2 / a0
    };

    return coeff;
}

template <int kChannels>
void Butterworth<kChannels>::process_frame(const NormalCoefficients& coeff, const float x[kChannels], float y[kChannels])
{
    for (uint32_t channel = 0; channel < kChannels; channel++)
    {
        // // TODO move this saturation to subclass Process each channel with resonance feedback
        // float input = x[channel] - fb_amount * tb303_tanh(state[channel].fb * 0.6f);
        float input = x[channel];

        // Filter
        float yn = coeff.b0 * input + coeff.b1 * state[channel].x[0] + coeff.b2 * state[channel].x[1]
                    - coeff.a1 * state[channel].y[0] - coeff.a2 * state[channel].y[1];
        
        // Update feedback state
        state[channel].x[1] = state[channel].x[0];
        state[channel].x[0] = input;
        state[channel].y[1] = state[channel].y[0];
        state[channel].y[0] = yn;
        state[channel].fb = yn;
    }
}