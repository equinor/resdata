#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <double_vector.h>

double statistics_empirical_quantile( double_vector_type * data , double quantile );

#ifdef __cplusplus
}
#endif
#endif
