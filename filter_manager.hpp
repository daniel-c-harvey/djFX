#include "butterworth.hpp"
#include "ui.hpp"

struct FilterManager {
    static UserParameters& get_params() {
        static UserParameters params;
        return params;
    }
    
    static ButterworthParameters& get_butterworth_params() {
        static ButterworthParameters params;
        return params;
    }
    
    static CompensatedParameters& get_compensated_params() {
        static CompensatedParameters params;
        return params;
    }
    
    static SaturatedParameters& get_saturated_params() {
        static SaturatedParameters params;
        return params;
    }
    
    static ButterworthHP<2, FilterParameters>& get_butterworth() {
        static ButterworthHP<2, FilterParameters> filter(k_samplerate, &get_butterworth_params());
        return filter;
    }
    
    static Compensated<2, FilterParameters>& get_compensated() {
        static Compensated<2, FilterParameters> filter(&get_butterworth(), &get_compensated_params());
        return filter;
    }
    
    static Saturated<2, FilterParameters>& get_saturated() {
        static Saturated<2, FilterParameters> filter(&get_compensated(), &get_saturated_params());
        return filter;
    }
    
    static FilterBase<2, FeedbackLine, NormalCoefficients, FilterParameters>* get_filter() {
        return &get_saturated();
    }
};