#include <math.h>
#include <stdlib.h>
#include <util.h>
#include <double_vector.h>



double statistics_empirical_quantile( double_vector_type * data , double quantile ) {
  if ((quantile < 0) || (quantile > 1.0))
    util_abort("%s: quantile must be in [0,1] \n",__func__);

  {
    const int size = (double_vector_size( data ) - 1);
    double value;
    double lower_value;
    double upper_value;
    double real_index;
    double upper_quantile;
    double lower_quantile;

    int    lower_index;
    int    upper_index;
    double_vector_sort( data );

    real_index  = quantile * size;
    lower_index = floor( real_index );
    upper_index = ceil( real_index );

    upper_quantile = upper_index * 1.0 / size;
    lower_quantile = lower_index * 1.0 / size;
    upper_value    = double_vector_iget( data , upper_index );
    lower_value    = double_vector_iget( data , lower_index );

    /* Linear interpolation: */
    {
      double a = (upper_value - lower_value) / (upper_quantile - lower_quantile);
      
      value = lower_value + a*(quantile - lower_quantile);
      return value;
    }
  }
}
