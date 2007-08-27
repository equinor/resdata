#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ecl_well_vars.h>


static bool string_cmp_list(const char * var , int len , const char ** list) {
  bool eq = false;
  int i;
  for (i=0; i < len; i++)
    if (strcmp(var, list[i]) == 0) eq = true;
  return eq;
}



static well_var_type __ecl_well_var_get_type(const char *var , bool *invalid , bool *history) {
  well_var_type type;

  if (string_cmp_list(var , 4 , (const char *[4]) {"OPR" , "ORAT" , "WOPR" , "WOPRH"})) 
    type = well_var_orat;
  else if (string_cmp_list(var , 4 , (const char *[4]) {"GPR" , "GRAT" , "WGPR" , "WGPRH"})) 
    type = well_var_grat;
  else if (string_cmp_list(var , 4 , (const char *[4]) {"WPR" , "WRAT" , "WWPR" , "WWPRH"})) 
    type = well_var_wrat;
  else if (string_cmp_list(var , 2 , (const char *[2]) {"BHP" , "WBHP"}))
    type = well_var_bhp;
  else if (string_cmp_list(var , 2 , (const char *[2]) {"THP" , "WTHP"}))
    type = well_var_thp;
  else if (string_cmp_list(var , 3 , (const char *[3]) {"WCT" , "WWCT" , "WWCTH"}))
    type = well_var_wct;
  else if (string_cmp_list(var , 3 , (const char *[3]) {"GOR" , "WGOR" , "WGORH"}))
    type = well_var_gor;
  else {
    *invalid = true;
    type = -1;
  }
  if (var[strlen(var) - 1] == 'H')
    *history = true;
  else
    *history = false;
  
  return type;
}




bool ecl_well_var_valid(const char * var) {
  bool invalid;
  bool history;
  __ecl_well_var_get_type(var , &invalid , &history);
  if (invalid)
    return false;
  else
    return true;
}


well_var_type ecl_well_var_get_type(const char * var) {
  bool invalid;
  bool history;
  well_var_type type;
  type = __ecl_well_var_get_type(var , &invalid , &history);
  if (invalid) {
    fprintf(stderr,"%s: well variables: %s not recognized - aborting \n",__func__ , var);
    abort();
  }
  return type;
}
