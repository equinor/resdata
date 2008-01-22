#include <stdlib.h>
#include <drivers.h>
#include <enkf_node.h>
#include <basic_driver.h>

#define PLAIN_DRIVER_ID 1001

struct plain_driver_struct {
  BASIC_DRIVER_FIELDS;
  int             plain_driver_id;
  path_fmt_type * path;
};


static plain_driver_assert_cast(plain_driver_type * plain_driver) {
  if (plain_driver->plain_driver_id != PLAIN_DRIVER_ID) {
    fprintf(stderr,"%s: internal error - cast failed - aborting \n",__func__);
    abort();
  }
}


void plain_driver_load_node(void * _driver , int report_step , int iens , bool analyzed , enkf_node_type * enkf_node) {
  plain_driver_type * driver = (plain_driver_type *) _driver;
  plain_driver_assert_cast(driver);
  {
    char * filename;
    enkf_node_fread(node , filename);
  }
}


void plain_driver_save_node(void * _driver , int report_step , int iens , bool analyzed , enkf_node_type * enkf_node) {
  plain_driver_type * driver = (plain_driver_type *) _driver;
  plain_driver_assert_cast(driver);
  {
    char * filename;
    enkf_node_fwrite(node , filename);
  }
}


void plain_driver_swapout_node(void * _driver , int report_step , int iens , bool analyzed , enkf_node_type * enkf_node) {
  plain_driver_type * driver = (plain_driver_type *) _driver;
  plain_driver_assert_cast(driver);
  {
    char * filename;
    enkf_node_swapout(node , filename);
  }
}


void plain_driver_swapin_node(void * _driver , int report_step , int iens , bool analyzed , enkf_node_type * enkf_node) {
  plain_driver_type * driver = (plain_driver_type *) _driver;
  plain_driver_assert_cast(driver);
  {
    char * filename;
    enkf_node_swapin(node , filename);
  }
}


plain_driver_type * plain_driver_alloc(path_fmt_type * path) {
  plain_driver_type * driver = malloc(sizeof * driver);
  driver->load    = plain_driver_load_node;
  driver->save    = plain_driver_save_node;
  driver->swapout = plain_driver_swapout_node;
  driver->swapin  = plain_driver_swapin_node;
  driver->plain_driver_id = PLAIN_DRIVER_ID;
  {
    basic_driver_type * basic_driver = (basic_driver_type *) driver;
    basic_driver_init(basic_driver);
    return basic_driver;
  }
}
