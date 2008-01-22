#include <stdlib.h>
#include <util.h>
#include <enkf_fs.h>
#include <path_fmt.h>
#include <enkf_node.h>
#include <fs_driver.h>
#include "fs_driver_decl.h"
#include "drivers.h"

struct enkf_fs_struct {
  fs_driver_type * dynamic_analyzed;
  fs_driver_type * dynamic_forecast;
  fs_driver_type * eclipse_static;
  fs_driver_type * parameter;
};




enkf_fs_type * enkf_fs_alloc(path_fmt_type * dynamic_analyzed_path , path_fmt_type * dynamic_forecast_path , path_fmt_type * eclipse_static_path , path_fmt_type * parameter_path) {
  enkf_fs_type * fs = malloc(sizeof * fs);

  fs->dynamic_analyzed  = fs_driver_alloc(dynamic_analyzed_path  , drivers_plain_load_node , drivers_plain_save_node , drivers_plain_swapin_node , drivers_plain_swapout_node);
  fs->dynamic_forecast  = fs_driver_alloc(dynamic_forecast_path  , drivers_plain_load_node , drivers_plain_save_node , drivers_plain_swapin_node , drivers_plain_swapout_node);
  fs->eclipse_static    = fs_driver_alloc(eclipse_static_path    , drivers_plain_load_node , drivers_plain_save_node , drivers_plain_swapin_node , drivers_plain_swapout_node);
  fs->parameter         = fs_driver_alloc(parameter_path         , drivers_plain_load_node , drivers_plain_save_node , drivers_plain_swapin_node , drivers_plain_swapout_node);
  
  return fs;
}


void enkf_fs_free(enkf_fs_type * fs) {
  fs_driver_free(fs->dynamic_analyzed);
  fs_driver_free(fs->dynamic_forecast);
  fs_driver_free(fs->eclipse_static);
  fs_driver_free(fs->parameter);
}


static fs_driver_type * enkf_fs_select_driver(enkf_fs_type * fs , const enkf_node_type * enkf_node , bool analyzed) {
  enkf_var_type var_type = enkf_node_get_var_type(enkf_node);
  fs_driver_type * driver;
  switch (var_type) {
  case(constant):
    driver = fs->parameter;
    break;
  case(static_parameter):
    driver = fs->parameter;
    break;
  case(parameter):
    driver = fs->parameter;
    break;
  case(ecl_restart):
    if (analyzed)
      driver = fs->dynamic_analyzed;
    else
      driver = fs->dynamic_forecast;
    break;
  case(ecl_summary):
    if (analyzed)
      driver = fs->dynamic_analyzed;
    else
      driver = fs->dynamic_forecast;
    break;
  case(ecl_static):
    driver = fs->eclipse_static;
    break;
  default:
    fprintf(stderr,"%s: fatal internal error - could not determine enkf_fs driver for object - aborting:\n",__func__);
    enkf_node_printf(enkf_node);
    abort();
  }
  return driver;
}



void enkf_fs_swapin_node(enkf_fs_type * enkf_fs , enkf_node_type * enkf_node , int report_step , int iens , bool analyzed) {
  fs_driver_type * driver = enkf_fs_select_driver(enkf_fs , enkf_node , analyzed);
  fs_driver_swapin_node(driver , report_step , iens , analyzed , enkf_node);
}


void enkf_fs_swapout_node(enkf_fs_type * enkf_fs , enkf_node_type * enkf_node , int report_step , int iens , bool analyzed) {
  fs_driver_type * driver = enkf_fs_select_driver(enkf_fs , enkf_node , analyzed);
  fs_driver_swapout_node(driver , report_step , iens , analyzed , enkf_node);
}

