#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <string.h>
#include <ecl_util.h>

static const char ecl_type_map1[6][5] = {{"CHAR\0"},
					 {"REAL\0"},
					 {"DOUB\0"},
					 {"INTE\0"},
					 {"LOGI\0"},
					 {"MESS\0"}};



static const char *ecl_type_map2[] = {"CHAR","REAL","DOUB","INTE","LOGI","MESS"};




int main(int argc, char ** argv) {
  int i;
  for (i=0; i < 6; i++) 
    printf("%d:  <%s:%d>  <%s:%d>  \n",i , ecl_type_map1[i] , strlen(ecl_type_map1[i]) , ecl_type_map2[i] , strlen(ecl_type_map2[i]));


  {
    ecl_file_type file_type;
    bool fmt_file;
    int report_nr;

    ecl_util_get_file_type("/tmp/kast/ECLIPSE.X0065"  , &file_type , &fmt_file , &report_nr);
    ecl_util_get_file_type("/tmp/kast/ECLIPSE.FUNRST" , &file_type , &fmt_file , &report_nr);
  }

  
  return 0;
}
