#include <math.h>
#include <trans_func.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <void_arg.h>
#include <util.h>


double trans_tanh(double x, const void_arg_type * arg) { return 0.5*(1 + tanh(x)); }

double trans_pow10(double x, const void_arg_type * arg) { return exp(log(10.0) * x); }

double trans_exp(double x, const void_arg_type * arg) { return exp(x); }

double trans_errf(double x, const void_arg_type * arg) { return 0.5*(1 + erf(x/sqrt(2.0))); }

double trans_step(double x, const void_arg_type * arg) { return (x < 0) ? 0 : 1; }

double trans_const(double x , const void_arg_type * arg) { return void_arg_get_double(arg , 0); }

double trans_unif(double x , const void_arg_type * arg) { abort(); return -1; }

double trans_derrf(double x , const void_arg_type * arg) {
  int steps;
  double y;
  steps = void_arg_get_int(arg , 0);
  y = floor(steps*0.5*(1 + erf(x/sqrt(2.0)))) / steps;
  return y;
}


double trans_dunif(double x , const void_arg_type * arg) {
  double y;
  int    steps = void_arg_get_int(arg , 0);
  double min   = void_arg_get_double(arg , 1);
  double max   = void_arg_get_double(arg , 2);
  
  y = 0.5*(1 + erf(x/sqrt(2.0))); /* 0 - 1 */
  return (floor( y * steps) / (steps - 1)) * (max - min) + min;
}


double trans_normal(double x , const void_arg_type * arg) {
  double mu , std;
  mu  = void_arg_get_double(arg , 0 );
  std = void_arg_get_double(arg , 1 );
  return x * std + mu;
}


transform_ftype * trans_func_lookup(FILE * stream , char ** _func_name , void_arg_type **_void_arg , bool *active) {
  char            * func_name;
  void_arg_type   * void_arg = NULL;
  transform_ftype * transf;

  *active = true;
  func_name = util_fscanf_alloc_token(stream);
  if (func_name == NULL) {
    fprintf(stderr,"%s: could not locate name of transformation - aborting \n",__func__);
    abort();
  }

  if (strcmp(func_name , "NORMAL") == 0) {
    /* Normal distribution */
    /* NORMAL mu std       */
    transf   = trans_normal;
    void_arg = void_arg_alloc2(double_value , double_value);
  } else if (strcmp(func_name , "UNIFORM") == 0) {
    /* Uniform distribution */
    /* UNIFORM min max      */
    transf   = trans_unif;
    void_arg = void_arg_alloc2(double_value , double_value);
  } else if (strcmp(func_name , "DUNIF") == 0) {
    /* Uniform distribution */
    /* DUNIF steps min max */
    transf   = trans_unif;
    void_arg = void_arg_alloc3(int_value , double_value , double_value);
  } else if (strcmp(func_name , "CONST") == 0) {
    /* Constant    */
    /* CONST value */
    transf   = trans_const;
    void_arg = void_arg_alloc1( double_value );
    *active  = false;
  } else {
    fprintf(stderr,"%s: function name:%s not recognized - aborting \n", __func__ , func_name);
    abort();
  }
  void_arg_fscanf(void_arg , stream);
  
  *_func_name = func_name;
  *_void_arg  = void_arg;
  return transf;
}


transform_ftype * trans_func_lookup_old(const char * func_name , FILE * stream , void_arg_type **void_arg) {
  *void_arg = NULL;
  if (func_name == NULL) 
    return NULL;
  else {
    if (strcmp(func_name , "TANH") == 0)
      return trans_tanh;
    else if (strcmp(func_name , "ERRF") == 0)
      return trans_errf;
    else if (strcmp(func_name , "DUNIF") == 0)
      return trans_dunif;
    else if (strcmp(func_name , "POW10") == 0)
      return trans_pow10;
    else if (strcmp(func_name , "EXP") == 0)
      return trans_exp;
    else if (strcmp(func_name , "STEP") == 0)
      return trans_step;
    else if (strcmp(func_name , "NULL") == 0)
      return NULL;
    else if (strcmp(func_name , "NONE") == 0)
      return NULL;
    else {
      fprintf(stderr,"%s: function name:%s not recognized - aborting \n", __func__ , func_name);
      abort();
    }
    /*
      Should do fscanf after arguments here ... 
    */
  }
}





