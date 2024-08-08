/*
  Warning: The resdata code has changed to be compiled as a C++ project. This
  header file is retained for a period for compatibility, but you are encouraged
  to switch to include the new hpp header directly in your code.
*/

#include <stdbool.h>
#include <stdio.h>

#ifndef ERT_SMSPEC_NODE_H
#define ERT_SMSPEC_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#define DUMMY_WELL ":+:+:+:+"
#define IS_DUMMY_WELL(well) (strcmp((well), DUMMY_WELL) == 0)
#define SMSPEC_PARAMS_INDEX_INVALID -77

#define SMSPEC_TIME_KEYWORD "TIME"
#define SMSPEC_TIME_NUMS_VALUE -32676

#define SMSPEC_YEARS_KEYWORD "YEARS"
#define SMSPEC_YEARS_NUMS_VALUE -32676

typedef enum {
    RD_SMSPEC_INVALID_VAR = 0,
    RD_SMSPEC_FIELD_VAR = 1,   /* X */
    RD_SMSPEC_REGION_VAR = 2,  /* X */
    RD_SMSPEC_GROUP_VAR = 3,   /* X */
    RD_SMSPEC_WELL_VAR = 4,    /* X */
    RD_SMSPEC_SEGMENT_VAR = 5, /* X */
    RD_SMSPEC_BLOCK_VAR = 6,   /* X */
    RD_SMSPEC_AQUIFER_VAR = 7,
    RD_SMSPEC_COMPLETION_VAR = 8, /* X */
    RD_SMSPEC_NETWORK_VAR = 9,
    RD_SMSPEC_REGION_2_REGION_VAR = 10,
    RD_SMSPEC_LOCAL_BLOCK_VAR = 11,      /* X */
    RD_SMSPEC_LOCAL_COMPLETION_VAR = 12, /* X */
    RD_SMSPEC_LOCAL_WELL_VAR = 13,       /* X */
    RD_SMSPEC_MISC_VAR = 14              /* X */
} rd_smspec_var_type;

#define SMSPEC_NUMS_INVALID -991199
#define SMSPEC_NUMS_WELL 1
#define SMSPEC_NUMS_GROUP 2
#define SMSPEC_NUMS_FIELD 0

#define SMSPEC_TYPE_ID 61550451

char *smspec_alloc_well_key(const char *join_string, const char *keyword,
                            const char *wgname);
bool smspec_node_identify_total(const char *keyword,
                                rd_smspec_var_type var_type);
bool smspec_node_identify_rate(const char *keyword);
int smspec_node_get_params_index(const void *smspec_node);
const char *smspec_node_get_gen_key1(const void *smspec_node);
const char *smspec_node_get_gen_key2(const void *smspec_node);
rd_smspec_var_type smspec_node_get_var_type(const void *smspec_node);
int smspec_node_get_num(const void *smspec_node);
const char *smspec_node_get_wgname(const void *smspec_node);
const char *smspec_node_get_keyword(const void *smspec_node);
const char *smspec_node_get_unit(const void *smspec_node);
bool smspec_node_is_rate(const void *smspec_node);
bool smspec_node_is_total(const void *smspec_node);
bool smspec_node_is_historical(const void *smspec_node);
bool smspec_node_need_nums(const void *smspec_node);
float smspec_node_get_default(const void *smspec_node);

bool smspec_node_lt(const void *node1, const void *node2);
int smspec_node_cmp(const void *node1, const void *node2);

#ifdef __cplusplus
}
#endif
#endif
