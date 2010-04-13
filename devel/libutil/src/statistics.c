#include <math.h>
#include <stdlib.h>
#include <util.h>
#include <double_vector.h>



double statistics_mean( const double_vector_type * data_vector ) {
  const double * data = double_vector_get_const_ptr( data_vector );
  int size = double_vector_size( data_vector );
  double sum = 0;
  for (int i=0; i < size; i++)
    sum += data[i];

  return sum / size;
}
 

double statistics_empirical_quantile( double_vector_type * data , double quantile ) {
  if ((quantile < 0) || (quantile > 1.0))
    util_abort("%s: quantile must be in [0,1] \n",__func__);

  {
    const int size = (double_vector_size( data ) - 1);
    double_vector_sort( data );
    
    if (double_vector_iget( data , 0) == double_vector_iget( data , size))
      /* 
         All elements are equal - and it is impossible to find a meaingful quantile,
         we just return "the value".
      */
      return double_vector_iget( data, 0 );    
    else {
      double value;
      double lower_value;
      double upper_value;
      double real_index;
      double upper_quantile;
      double lower_quantile;
      
      int    lower_index;
      int    upper_index;
      
      
      real_index  = quantile * size;
      lower_index = floor( real_index );
      upper_index = ceil( real_index );
      
      upper_value    = double_vector_iget( data , upper_index );
      lower_value    = double_vector_iget( data , lower_index );

      /* 
         Will iterate in this loop until we have found upper_value !=
         lower_value. As long as we know that now all elements are
         equal (the first test), this is guaranteed to succeed, but of
         course the estimate will not be very meaningful if the sample
         consist of a significant number of equal values.
      */
      while (true) {

        /*1: Try to shift the upper index up. */
        if (upper_value == lower_value) {
          upper_index = util_int_min( size , upper_index + 1);
          upper_value = double_vector_iget( data , upper_index );
        } else 
          break;

        /*2: Try to shift the lower index down. */
        if (upper_value == lower_value) {
          lower_index = util_int_max( 0 , lower_index - 1);
          lower_value = double_vector_iget( data , lower_index );
        } else 
          break;
        
      }
      
      upper_quantile = upper_index * 1.0 / size;
      lower_quantile = lower_index * 1.0 / size;
      /* Linear interpolation: */
      {
        double a = (upper_value - lower_value) / (upper_quantile - lower_quantile);
        
        value = lower_value + a*(quantile - lower_quantile);
        return value;
      }
    }
  }
}
