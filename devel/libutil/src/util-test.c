#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <stringlist.h>
#include <menu.h>
#include <subst.h>
#include <arg_pack.h>




int main(int argc , char ** argv) {
  int NAME_MAX = 20;
  int num_columns = 4;
  int num_rows    = 6;

  char   ** column_names = util_malloc(num_columns * sizeof * column_names, __func__);
  double ** data         = util_malloc(num_columns * sizeof * data,         __func__);

  for(int i=0; i<num_columns; i++)
  {
    data[i]         = util_malloc(num_rows * sizeof * data[i],        __func__);
    column_names[i] = util_malloc(NAME_MAX * sizeof * column_names[i], __func__);
    
    for(int j=0; j<num_rows; j++)
      data[i][j] = 0.0;

    sprintf(column_names[i], "col %i", i);
  }


  util_fprintf_data_summary(data, column_names, num_rows, num_columns, "foo");


  for(int i=0; i<num_columns; i++)
  {
    free(data[i]);
    free(column_names[i]);
  }
  free(data);

  return 0;
}
