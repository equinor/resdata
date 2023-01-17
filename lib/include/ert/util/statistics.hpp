#ifndef ERT_STATISTICS_H
#define ERT_STATISTICS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <ert/util/double_vector.hpp>

double statistics_std(const double_vector_type *data_vector);
double statistics_mean(const double_vector_type *data_vector);
double statistics_empirical_quantile(double_vector_type *data, double quantile);
double statistics_empirical_quantile__(const double_vector_type *data,
                                       double quantile);

#ifdef __cplusplus
}
#endif
#endif
