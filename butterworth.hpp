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
        void init(const uint32_t sample_rate);

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