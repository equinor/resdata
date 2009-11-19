#include <ecl_kw.h>
#include <stdlib.h>
#include <ecl_sum.h>
#include <util.h>
#include <string.h>
#include <signal.h>


void install_SIGNALS(void) {
  signal(SIGSEGV , util_abort_signal);    /* Segmentation violation, i.e. overwriting memory ... */
  signal(SIGINT  , util_abort_signal);    /* Control C */
  signal(SIGTERM , util_abort_signal);    /* If killing the enkf program with SIGTERM (the default kill signal) you will get a backtrace. Killing with SIGKILL (-9) will not give a backtrace.*/
}



int main(int argc , char ** argv) {
  install_SIGNALS();
  {
    if (argc < 3) {
      fprintf(stderr,"** ERROR ** %s ECLIPSE.DATA WOPR:WGNAME \n",__func__);
      exit(1);
    }
    {
      char * data_file;
      ecl_sum_type * ecl_sum;
      bool report_only = false;
      int offset = 2;
      
      if (strcmp(argv[1] , "-R") == 0) {
	report_only = true; 
	offset += 1;
	data_file = argv[2];
      } else
	data_file = argv[1];
      
      ecl_sum = ecl_sum_fread_alloc_case( data_file );
      ecl_sum_fprintf(ecl_sum , stdout , argc - offset , (const char **) &argv[offset] , report_only);
      ecl_sum_free(ecl_sum);
    }
  }
}
