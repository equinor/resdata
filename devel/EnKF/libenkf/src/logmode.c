#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <logmode.h>
#include <enkf_types.h>

struct logmode_struct {
  double     base;
  bool       log_input;
  bool       log_enkf;
  bool       log_output;
};



static bool __iand(enkf_logmode_enum logmode , enkf_logmode_enum mask) {
  if (logmode & mask)
    return true;
  else
    return false;
}

  

logmode_type * logmode_alloc(double base , enkf_logmode_enum mode) {
  logmode_type * logmode = malloc(sizeof * logmode);
  logmode->base = base;
  logmode->log_input  = __iand(mode , log_input_mask);
  logmode->log_enkf   = __iand(mode , log_enkf_mask);
  logmode->log_output = __iand(mode , log_output_mask);
  return logmode;
}


void logmode_free(logmode_type * logmode) {
  free(logmode);
}



static void logmode_transform_data(double base, bool src_log , bool target_log , int size, double * data) {
  int i;
  if (src_log) {
    if (!target_log)
      for (i=0; i < size; i++)
	data[i] = exp(log(base) * data[i]);
  } else {
    if (target_log)
      for (i=0; i < size; i++)
	data[i] = log(data[i]) / log(base);
  }
}



void logmode_transform_input_data(const logmode_type * logmode , int size, double *data) {
  logmode_transform_data(logmode->base, logmode->log_input , logmode->log_enkf , size , data);
}


void logmode_transform_output_data(const logmode_type * logmode , int size, double *data) {
  logmode_transform_data(logmode->base , logmode->log_enkf , logmode->log_output , size , data);
}



static void logmode_transform_distribution(double base , bool src_log , bool target_log , double mean , double std , double *_mu , double *_sigma) {
  double mu,sigma;

  if (src_log) {
    if (!target_log) {
      fprintf(stderr,"%s: not implemented - aborting \n",__func__);
      abort();
    }
  } else if (target_log) {
    sigma = sqrt(log(std*std / (mean*mean) + 1));
    mu    = log(mean) - std*std/2;

    *_sigma = sigma / log(base);
    *_mu    = mu    / log(base);
  }
}



/*
  A variable x is lognormal distributed with: <x> = mean and <(x - <x>)^2> = std*std

  Then the variable y = log x is normally distributed with <y> = sigma_N and <(y - <y>)^2> = mu_N * mu_N
*/

void logmode_transform_input_distribution(const logmode_type * logmode , double mean , double std , double *_mu , double *_sigma) {
  logmode_transform_distribution(logmode->base , logmode->log_input , logmode->log_enkf , mean , std , _mu , _sigma);
}

