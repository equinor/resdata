#include <stdio.h>
#include <stdlib.h>
#include <util.h>
#include <matrix.h>
#include <regression.h>
#include <stepwise.h>
#include <bool_vector.h>
#include <double_vector.h>



#define STEPWISE_TYPE_ID 8722106

struct stepwise_struct {
  UTIL_TYPE_ID_DECLARATION;

  matrix_type      * X0;             // Externally supplied data.
  matrix_type      * Y0;
  bool               data_owner;     // Does the stepwise estimator own the data matrices X0 and Y0?

  matrix_type      * beta;           // Quantities estimated by the stepwise algorithm
  double             y0; 
  bool_vector_type * active_set;
  rng_type         * rng;           // Needed in the cross-validation 
};








static double stepwise_estimate__( stepwise_type * stepwise , bool_vector_type * active_rows) {
  matrix_type * X;
  matrix_type * Y;

  double y_mean    = 0;
  int nvar         = matrix_get_columns( stepwise->X0 );
  int nsample      = matrix_get_rows( stepwise->X0 );

  nsample = bool_vector_count_equal( active_rows , true );
  nvar = bool_vector_count_equal( stepwise->active_set , true );
  
  matrix_set( stepwise->beta , 0 ); // It is essential to make sure that old finite values in the beta0 vector do not hang around.


  /*
    Extracting the data used for regression, and storing them in the
    temporary local matrices X and Y. Selecting data is based both on
    which varibles are active (stepwise->active_set) and which rows
    should be used for regression, versus which should be used for
    validation (@active_rows).
  */
  if ((nsample < matrix_get_rows( stepwise->X0 )) || (nvar < matrix_get_columns( stepwise->X0 ))) {
    X = matrix_alloc( nsample , nvar );
    Y = matrix_alloc( nsample , 1);
    
    {
      int icol,irow;   // Running over all values.
      int arow,acol;   // Running over active values.
      arow = 0;
      for (irow = 0; irow < matrix_get_rows( stepwise->X0 ); irow++) {
        if (bool_vector_iget( active_rows , irow )) {
          acol = 0;
          for (icol = 0; icol < matrix_get_columns( stepwise->X0 ); icol++) {
            if (bool_vector_iget( stepwise->active_set , icol )) {
              matrix_iset( X , arow , acol , matrix_iget( stepwise->X0 , irow , icol ));
              acol++;
            }
          }
          matrix_iset( Y , arow , 0 , matrix_iget( stepwise->Y0 , irow , 0 ));
          arow++;
        }
      }
    }
  } else {
    X = matrix_alloc_copy( stepwise->X0 );
    Y = matrix_alloc_copy( stepwise->Y0 );
  }
  
  
  {
    matrix_type * beta     = matrix_alloc( nvar , 1);           /* This is the beta vector as estimated from the OLS estimator. */
    matrix_type * tmp_beta = matrix_alloc_copy( beta );         /* This is the beta vector shifted and scaled back to original variables. */ 
    matrix_type * X_mean   = matrix_alloc( 1 , nvar );
    matrix_type * X_norm   = matrix_alloc( 1 , nvar );

    y_mean = regression_scale( X , Y , X_mean , X_norm );
    regression_OLS( X , Y , beta );
    y_mean = regression_unscale( beta , X_norm , X_mean , y_mean , tmp_beta );

    /* 
       In this code block the beta/tmp_beta vector which is dense with
       fewer elements than the full model is scattered into the beta0
       vector which has full size and @nvar elements.
    */
    {
      int ivar,avar;   
      avar = 0;
      for (ivar = 0; ivar < matrix_get_columns( stepwise->X0 ); ivar++) {
        if (bool_vector_iget( stepwise->active_set , ivar )) {
          matrix_iset( stepwise->beta , ivar , 0 , matrix_iget( tmp_beta , avar , 0));
          avar++;
        }
      }
    }
    
    matrix_free( X_mean );
    matrix_free( X_norm );
    matrix_free( beta );
    matrix_free( tmp_beta );
  }
  
  matrix_free( X );
  matrix_free( Y );
  return y_mean;
}




static double stepwise_eval__( const stepwise_type * stepwise , const matrix_type * x ) { 
  return stepwise->y0 + matrix_row_column_dot_product( x , 0 , stepwise->beta , 0 );
}



static double stepwise_test_var( stepwise_type * stepwise , int test_var , int blocks) {
  double prediction_error = 0;
  bool_vector_iset( stepwise->active_set , test_var , true );   // Temporarily activate this variable
  {

    int nvar                       = matrix_get_columns( stepwise->X0 );
    int nsample                    = matrix_get_rows( stepwise->X0 );
    int block_size                 = nsample / blocks;
    matrix_type * beta             = matrix_alloc( nvar , 1 );
    bool_vector_type * active_rows = bool_vector_alloc( nsample, true );

    /*True Cross-Validation: */
    int * randperms     = util_malloc( sizeof * randperms * nsample, __func__);
    for (int i=0; i < nsample; i++)
      randperms[i] = i;
    
    /* Randomly perturb ensemble indices */
    rng_shuffle_int( stepwise->rng , randperms , nsample );
    
    
    for (int iblock = 0; iblock < blocks; iblock++) {
      int validation_start = iblock * block_size;
      int validation_end   = validation_start + block_size - 1;
      
      if (iblock == (blocks - 1))
        validation_end = nsample - 1;

      /*
        Ensure that the active_rows vector has a block consisting of
        the interval [validation_start : validation_end] which is set to
        false, and the remaining part of the vector is set to true.
      */
      {
        bool_vector_reset( active_rows );
        bool_vector_iset( active_rows , nsample - 1 , true );   
        /* 
           If blocks == 1 that means all datapoint are used in the
           regression, and then subsequently reused in the R2
           calculation. 
        */
        if (blocks > 1) {                                      
          for (int i = validation_start; i <= validation_end; i++)
            bool_vector_iset( active_rows , randperms[i] , false );
        }
      }
      
  
      /*
        Evaluate the prediction error on the validation part of the
        dataset.
      */
      {
        stepwise_estimate__( stepwise , active_rows );
        {
          int irow;
          matrix_type * x_vector = matrix_alloc( 1 , nvar );
          for (irow=validation_start; irow <= validation_end; irow++) {
            matrix_copy_row( x_vector , stepwise->X0 , 0 , irow);
            
            {
              double true_value      = matrix_iget( stepwise->Y0 , irow , 0 );
              double estimated_value = stepwise_eval__( stepwise , x_vector );
              prediction_error += (true_value - estimated_value) * (true_value - estimated_value);
            }
            
          }
          matrix_free( x_vector );
        }
      }
    }
    
    matrix_free( beta );  
    free( randperms );
    bool_vector_free( active_rows );
  }
  
  /*inactivate the test_var-variable after completion*/
  bool_vector_iset( stepwise->active_set , test_var , false );  
  return prediction_error;
}






void stepwise_estimate( stepwise_type * stepwise , double deltaR2_limit , int CV_blocks) {
  int nvar          = matrix_get_columns( stepwise->X0 );
  int nsample       = matrix_get_rows( stepwise->X0 );
  double currentR2 = -1;
  double y0;
  bool_vector_type * active_rows = bool_vector_alloc( nsample , true );
  
  bool_vector_set_default( stepwise->active_set , false );
  bool_vector_reset( stepwise->active_set );
  bool_vector_iset( stepwise->active_set , nvar - 1 , false );

  while (true) {
    double minR2    = -1;
    int    best_var = 0;
    
    /*
      Go through all the inactive variables, and calculate the
      resulting prediction error IF this particular variable is added;
      keep track of the variable which gives the lowest prediction error.
    */
    for (int ivar = 0; ivar < nvar; ivar++) {
      if (!bool_vector_iget( stepwise->active_set , ivar)) {
        double newR2 = stepwise_test_var(stepwise , ivar , CV_blocks);
        if ((minR2 < 0) || (newR2 < minR2)) {
          minR2 = newR2;
          best_var = ivar;
        }
      }
    }


    /*
      If the best relative improvement in prediction error is better
      than @deltaR2_limit, the corresponding variable is added to the
      active set, and we return to repeat the loop one more
      time. Otherwise we just exit.
    */
    
    {
      double deltaR2 = (currentR2 - minR2)/currentR2;
      if (( currentR2 < 0) || deltaR2 > deltaR2_limit) {
        bool_vector_iset( stepwise->active_set , best_var , true );
        currentR2 = minR2;
        y0 = stepwise_estimate__( stepwise , active_rows );
      } else
        /* The gain in prediction error is so small that we just leave the building. */
        break; 
      
      if (bool_vector_count_equal( stepwise->active_set , true) == matrix_get_columns( stepwise->X0 ))
        break;   /* All variables are active. */
    }
  }
  bool_vector_free( active_rows );
}



double stepwise_eval( const stepwise_type * stepwise , const matrix_type * x ) { 
  double yHat = stepwise_eval__(stepwise, x );
  return yHat;
}



static stepwise_type * stepwise_alloc__( int nsample , int nvar) {
  stepwise_type * stepwise = util_malloc( sizeof * stepwise , __func__ );

  
  stepwise->X0          = NULL;
  stepwise->Y0          = NULL;
  stepwise->active_set  = bool_vector_alloc( nvar , true );
  stepwise->beta        = matrix_alloc( nvar , 1 );

  return stepwise;
}


stepwise_type * stepwise_alloc1( int nsample , int nvar, rng_type * rng) {
  stepwise_type * stepwise = stepwise_alloc__( nsample , nvar );

  stepwise->rng         = rng;
  stepwise->X0          = matrix_alloc( nsample , nvar );
  stepwise->Y0          = matrix_alloc( nsample , 1 );
  stepwise->data_owner  = true;
  
  return stepwise;
}


stepwise_type * stepwise_alloc2( matrix_type * X , matrix_type * Y , bool internal_copy ) {
  stepwise_type * stepwise = stepwise_alloc__( matrix_get_rows( X ) , matrix_get_columns( X ));
  if (internal_copy) {
    stepwise->X0 = matrix_alloc_copy( X );
    stepwise->Y0 = matrix_alloc_copy( Y );
    stepwise->data_owner = true;
  } else {
    stepwise->X0 = X; 
    stepwise->Y0 = Y;
    stepwise->data_owner = false;
  }
  return stepwise;
}


void stepwise_set_Y0( stepwise_type * stepwise , const matrix_type * Y) {
  stepwise->Y0 = Y;
}

void stepwise_set_X0( stepwise_type * stepwise , const matrix_type * X) {
  stepwise->X0 = X;
}



void stepwise_free( stepwise_type * stepwise ) {
  
  bool_vector_free( stepwise->active_set );
  matrix_free( stepwise->beta );
  if (stepwise->data_owner) {
    matrix_free( stepwise->X0 );
    matrix_free( stepwise->Y0 );
  }
  free( stepwise );
  
}
