#include <ecl_fstate.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <msg.h>
#include <stdbool.h>


void summary_file_get_last(const char * summary_file , msg_type * msg) {
  const bool fmt_file = false;
  ecl_kw_type * seqhdr_kw;
  ecl_kw_type * ministep_kw;
  ecl_kw_type * params_kw;
    
  fortio_type * fortio = fortio_open(summary_file , "r" , true);
  seqhdr_kw = ecl_kw_fread_alloc(fortio , fmt_file);

  {
    long int start_pos = 0;
    while (ecl_kw_fseek_kw("MINISTEP" , fmt_file , false , false , fortio)) {
      start_pos = ftell(fortio_get_FILE(fortio));
      ecl_kw_fskip(fortio , fmt_file);
    }
    fseek(fortio_get_FILE(fortio) , start_pos , SEEK_SET);
  }
  

  ministep_kw = ecl_kw_fread_alloc(fortio ,  fmt_file);
  params_kw   = ecl_kw_fread_alloc(fortio ,  fmt_file);
  fortio_close(fortio);

  fortio = fortio_open(summary_file , "w" , true);
  ecl_kw_fwrite(seqhdr_kw   , fortio);
  ecl_kw_fwrite(ministep_kw , fortio);
  ecl_kw_fwrite(params_kw   , fortio);
  fortio_close(fortio);
  msg_update(msg , summary_file);
}


int main(int argc , char **argv) {
  msg_type * msg = msg_alloc("Ferdig med: ");
  int i;

  msg_show(msg);
  for (i=1; i < argc; i++) {
    summary_file_get_last(argv[i] , msg);
  }
  msg_free(msg , false);
  return 0;
}
