#include <fs_driver.h>
#include "fs_driver_decl.h"
#include <stdio.h>
#include <util.h>
#include <path_fmt.h>
#include <enkf_node.h>
#include "drivers.h"



fs_driver_type * fs_driver_alloc(path_fmt_type * path , load_node_ftype * load , save_node_ftype * save , swapin_node_ftype * swapin , swapout_node_ftype * swapout) {
  fs_driver_type * fs_driver = malloc(sizeof * fs_driver);

  fs_driver->path    = path_fmt_copyc(path);
  fs_driver->load    = load;
  fs_driver->save    = save;
  fs_driver->swapin  = swapin;
  fs_driver->swapout = swapout;
  return fs_driver;
}


void fs_driver_save_node(fs_driver_type *driver , int report_step , int iens , bool analyzed , enkf_node_type * node) {
  char * filename = path_fmt_alloc_file(driver->path , report_step , iens);
  driver->save(node , filename);
  free(filename);
}



void fs_driver_load_node(fs_driver_type *driver , int report_step , int iens , bool analyzed , enkf_node_type * node) {
  char * filename = path_fmt_alloc_file(driver->path , report_step , iens);
  driver->load(node , filename);
  free(filename);
}



void fs_driver_swapin_node(fs_driver_type *driver , int report_step , int iens , bool analyzed , enkf_node_type * node) {
  char * filename = path_fmt_alloc_file(driver->path , report_step , iens);
  driver->swapin(node , filename);
  free(filename);
}



void fs_driver_swapout_node(fs_driver_type *driver , int report_step , int iens , bool analyzed , enkf_node_type * node) {
  char * filename = path_fmt_alloc_file(driver->path , report_step , iens);
  driver->swapout(node , filename);
  free(filename);
}




void fs_driver_free(fs_driver_type * fs_driver) {
  path_fmt_free(fs_driver->path);
}








