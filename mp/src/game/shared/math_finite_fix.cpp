// In glibc 2.31 these functions got removed, but bitmap and particles libraries still use it

#include <features.h>
#include <math.h>

#if __GLIBC_PREREQ(2, 31)

extern "C" {
    double __exp_finite(double x) { return exp(x); }
    double __log_finite(double x) { return log(x); }
    double __pow_finite(double x, double y) { return pow(x, y); }
    double __atan2_finite(double x, double y) { return atan2(x, y); }
    double __asin_finite(double x) { return sin(x); }
    double __acos_finite(double x) { return cos(x); }

    float __expf_finite(float x) { return expf(x); }
    float __logf_finite(float x) { return logf(x); }
    float __powf_finite(float x, float y) { return powf(x, y); }
    float __acosf_finite(float x) { return acosf(x); }
    float __atan2f_finite(float x, float y) { return atan2f(x, y); }
}

#endif