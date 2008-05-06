#include <ecl_kw.h>
#include <stdlib.h>
#include <ecl_sum.h>
#include <util.h>


int main(int argc , char ** argv) {
  if (argc < 3) {
    fprintf(stderr,"** ERROR ** %s ECLIPSE.DATA WOPR:WGNAME \n",__func__);
    exit(1);
  }
  {
    const char * data_file = argv[1];
    char  * base;
    char  * header_file;
    char  * path;
    char ** summary_file_list;
    int     files;
    bool    fmt_file , unified;
    ecl_sum_type * ecl_sum;

    util_alloc_file_components( data_file , &path , &base , NULL);
    ecl_util_alloc_summary_files( path , base , &header_file , &summary_file_list , &files , &fmt_file , &unified);
    ecl_sum = ecl_sum_fread_alloc( header_file , files , (const char **) summary_file_list , true , true );
    ecl_sum_fprintf(ecl_sum , stdout , argc - 2 , (const char **) &argv[2]);
  }
    

}
