#ifndef __LAYER_CONFIG_H__
#define __LAYER_CONFIG_H__
#include <config.h>
#include <field_config.h>

typedef struct layer_config_struct layer_config_type;

struct layer_config_struct {
  CONFIG_STD_FIELDS;
  int  iz;
  int  nx,ny;
  int  logmode;
  bool compact;
  bool data_owner;
  const int *index_map;
  int        index_map_offset;
  const field_config_type * target_config;
};



#endif
