#pragma once

#include "usermodfx.h"
#include "util.hpp"
#include <memory>

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
class Filter
{
    public:        
        /// @brief Prepare the filter channels to process all frames in this block
        /// @param param_cutoff [0, 1]
        /// @param param_resonance [0, 1]
        virtual TParams prepare_parameters(const float param_cutoff, const float param_resonance) = 0;
        
        /// @brief Prepare the filter channels to process all frames in this block
        virtual TCoefficients prepare_coefficients(const TParams& params) = 0;
        
        /// @brief process the current frame samples for all channels
        /// @param x inputs samples
        /// @param y output samples
        virtual void process_frame(const TCoefficients& coeff, const TParams& params, 
                                   const float x[k_channels], float y[k_channels]) = 0;
        
    protected:
        /// @brief process the current frame sample for given channel
        /// @param x inputs sample
        /// @param y output sample
        virtual void process_channel_frame(TFeedbackLine& state, const TCoefficients& coeff, 
                                           const TParams& params, const float& x, float& y) = 0;
};

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TParams>
class FilterDecorator : public Filter<k_channels, TFeedbackLine, TCoefficients, TParams>
{
    public:
        FilterDecorator(std::unique_ptr<Filter<k_channels, TFeedbackLine, TCoefficients, TParams>> f);

        TCoefficients prepare_coefficients(const TParams& params) override;

        void process_frame(const TCoefficients& coeff, const TParams& params, 
                           const float x[k_channels], float y[k_channels]) override;
    protected:
        std::unique_ptr<Filter<k_channels, TFeedbackLine, TCoefficients, TParams>> filter_ptr;
};

struct ButterworthParameters 
{
    float cutoff;
    float res;
    float Q;
};

typedef struct {
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
} NormalCoefficients;

typedef struct {
    float x[2];  // Previous inputs
    float y[2];  // Previous outputs
    float fb;  // Feedback value for resonance
} FeedbackLine;

template <int k_channels, typename TParams>
class ButterworthHP : public Filter<k_channels, FeedbackLine, NormalCoefficients, TParams>
{
    public:
        explicit ButterworthHP(const unsigned long& sample_rate);

        virtual TParams prepare_parameters(const float param_cutoff, 
                                           const float param_resonance) override;
                
        virtual NormalCoefficients prepare_coefficients(const TParams& params) override;
        
        virtual void process_frame(const NormalCoefficients& coeff, 
                                   const TParams& params, 
                                   const float x[k_channels], 
                                   float y[k_channels]) override;
    protected:
        uint32_t sample_rate;
        FeedbackLine state[k_channels];

        virtual void process_channel_frame(FeedbackLine& state, 
                                           const NormalCoefficients& coeff, 
                                           const TParams& params, 
                                           const float& x, 
                                           float& y) override;
};

// template <int k_channels>
// class Butterworth : public ButterworthHP<k_channels, Parameters> {};

struct CompensatedParameters : public ButterworthParameters
{
    float vol_comp; // Compensate for resonance-induced volume loss
    float fb_amount; // feedback based on resonance
};

// template <int k_channels, typename TParams>
// class __CompensatedButterworth : public ButterworthHP<k_channels, TParams> 
// {
//     public:
//         virtual TParams prepare_parameters(const float param_cutoff, 
//                                            const float param_resonance) override;
//     protected:
//         virtual void process_channel_frame(FeedbackLine& state, 
//                                            const NormalCoefficients& coeff, 
//                                            const TParams& params, 
//                                            const float& x, 
//                                            float& y) override;
// };

template <int k_channels>
class Compensated : public FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, CompensatedParameters> 
{
    public:
        explicit Compensated(std::unique_ptr<Filter<k_channels, FeedbackLine, NormalCoefficients, CompensatedParameters>> f)
        : FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, CompensatedParameters>(std::move(f)) {}

        CompensatedParameters prepare_parameters(const float param_cutoff, 
                                                 const float param_resonance) override;
    protected:
        void process_channel_frame(FeedbackLine& state, 
                                   const NormalCoefficients& coeff, 
                                   const CompensatedParameters& params, 
                                   const float& x, 
                                   float& y) override;
};

// template <int k_channels>
// class CompensatedButterworth : public __CompensatedButterworth<k_channels, CompensatedParameters> {};

struct SaturatedParameters : public CompensatedParameters
{
    float drive;
};

// template <int k_channels, typename TParams>
// class __SaturatedButterworth : public __CompensatedButterworth<k_channels, TParams>
// {
//     public:
//         virtual TParams prepare_parameters(const float param_cutoff, 
//                                            const float param_resonance) override;
//     protected:
//         void process_channel_frame(FeedbackLine& state, 
//                                    const NormalCoefficients& coeff, 
//                                    const TParams& params, 
//                                    const float& x, 
//                                    float& y) override;
// };

template <int k_channels>
class Saturated : public FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, SaturatedParameters> 
{
    public:
        explicit Saturated(std::unique_ptr<Filter<k_channels, FeedbackLine, NormalCoefficients, SaturatedParameters>> f)
        : FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, SaturatedParameters>(std::move(f)) {}

        SaturatedParameters prepare_parameters(const float param_cutoff, 
                                                       const float param_resonance) override;
    protected:
        void process_channel_frame(FeedbackLine& state, 
                                   const NormalCoefficients& coeff, 
                                   const SaturatedParameters& params, 
                                   const float& x, 
                                   float& y) override;
};

// template <int k_channels>
// class SaturatedButterworth : public __SaturatedButterworth<k_channels, SaturatedParameters> {};

#include "butterworth.tpp"