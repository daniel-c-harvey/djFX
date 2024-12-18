#pragma once

#include "usermodfx.h"
#include "util.hpp"
#include <memory>

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TUIParams>
class FilterBase
{
    public:        
        /// @brief Prepare the filter channels to process all frames in this block
        virtual void prepare_parameters(const TUIParams& params) = 0;

        /// @brief Prepare the filter channels to process all frames in this block
        virtual TCoefficients prepare_coefficients() = 0;
                
        /// @brief process the current frame samples for all channels
        /// @param x inputs samples
        /// @param y output samples
        virtual void process_frame(const TCoefficients& coeff, const float x[k_channels], float y[k_channels]) = 0;
        
        /// @brief process the current frame sample for given channel
        /// @param x inputs sample
        /// @param y output sample
        virtual void process_channel_frame(TFeedbackLine& state, const TCoefficients& coeff, const float& x, float& y) = 0;
};

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TUIParams, typename TFilterParams>
class Filter : public FilterBase<k_channels, TFeedbackLine, TCoefficients, TUIParams>
{
    public:
        explicit Filter(TFilterParams *p);

    protected:
        TFilterParams* params;
};

template <int k_channels, typename TFeedbackLine, typename TCoefficients, typename TUIParams, typename TFilterParams>
class FilterDecorator : public Filter<k_channels, TFeedbackLine, TCoefficients, TUIParams, TFilterParams>
{
    public:
        FilterDecorator(FilterBase<k_channels, TFeedbackLine, TCoefficients, TUIParams> *f, TFilterParams *p);

        void prepare_parameters(const TUIParams& params) override;

        TCoefficients prepare_coefficients() override;

        void process_frame(const TCoefficients& coeff, const float x[k_channels], float y[k_channels]) override;

    protected:
        FilterBase<k_channels, TFeedbackLine, TCoefficients, TUIParams> *filter_ptr;

        void process_channel_frame(TFeedbackLine& state, const TCoefficients& coeff, const float& x, float& y) override;
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

template <int k_channels, typename TUIParams>
class ButterworthHP : public Filter<k_channels, FeedbackLine, NormalCoefficients, TUIParams, ButterworthParameters>
{
    public:
        ButterworthHP(const unsigned long& sample_rate, ButterworthParameters *params);

        void prepare_parameters(const TUIParams& params) override;

        NormalCoefficients prepare_coefficients() override;
        
        void process_frame(const NormalCoefficients& coeff, 
                           const float x[k_channels], 
                           float y[k_channels]) override;
    protected:
        uint32_t sample_rate;
        FeedbackLine state[k_channels];


        void process_channel_frame(FeedbackLine& state, 
                                   const NormalCoefficients& coeff, 
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

// template <int k_channels, typename TFilterParams>
// class __CompensatedButterworth : public ButterworthHP<k_channels, TFilterParams> 
// {
//     public:
//         virtual TFilterParams prepare_parameters(const float param_cutoff, 
//                                            const float param_resonance) override;
//     protected:
//         virtual void process_channel_frame(FeedbackLine& state, 
//                                            const NormalCoefficients& coeff, 
//                                            const TFilterParams& params, 
//                                            const float& x, 
//                                            float& y) override;
// };

template <int k_channels, typename TUIParams>
class Compensated : public FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, TUIParams, CompensatedParameters> 
{
    public:
        Compensated(FilterBase<k_channels, FeedbackLine, NormalCoefficients, TUIParams> *f, CompensatedParameters *p)
        : FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, TUIParams, CompensatedParameters>(f, p) {}

        void prepare_parameters(const TUIParams& params) override;

        // void process_frame(const NormalCoefficients& coeff, const float x[k_channels], float y[k_channels]) override;

    protected:
        void process_channel_frame(FeedbackLine& state, 
                                   const NormalCoefficients& coeff,
                                   const float& x, 
                                   float& y) override;
};

// template <int k_channels>
// class CompensatedButterworth : public __CompensatedButterworth<k_channels, CompensatedParameters> {};

struct SaturatedParameters : public CompensatedParameters
{
    float drive;
};

// template <int k_channels, typename TFilterParams>
// class __SaturatedButterworth : public __CompensatedButterworth<k_channels, TFilterParams>
// {
//     public:
//         void process_frame(const NormalCoefficients& coeff, const float x[k_channels], float y[k_channels]) override;

//     protected:
//         void process_channel_frame(FeedbackLine& state, 
//                                    const NormalCoefficients& coeff, 
//                                    const float& x, 
//                                    float& y) override;
// };

template <int k_channels, typename TUIParams>
class Saturated : public FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, TUIParams, SaturatedParameters> 
{
    public:
        Saturated(FilterBase<k_channels, FeedbackLine, NormalCoefficients, TUIParams> *f, SaturatedParameters *p)
        : FilterDecorator<k_channels, FeedbackLine, NormalCoefficients, TUIParams, SaturatedParameters>(f, p) {}

        void prepare_parameters(const TUIParams& params) override;

        // void process_frame(const NormalCoefficients& coeff, const float x[k_channels], float y[k_channels]) override;

    protected:
        void process_channel_frame(FeedbackLine& state, 
                                   const NormalCoefficients& coeff,
                                   const float& x, 
                                   float& y) override;
};

// template <int k_channels>
// class SaturatedButterworth : public __SaturatedButterworth<k_channels, SaturatedParameters> {};

#include "butterworth.tpp"