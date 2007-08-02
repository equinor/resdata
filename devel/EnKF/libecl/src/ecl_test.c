#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <string.h>

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

  return 0;
}
