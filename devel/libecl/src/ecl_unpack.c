#include <stdbool.h>
#include <util.h>
#include <ecl_file.h>
#include <ecl_util.h>
#include <ecl_kw.h>
#include <msg.h>

void unpack_file(const char * filename) {
  ecl_file_enum file_type , target_type;
  bool fmt_file;
  ecl_util_get_file_type(filename , &file_type , &fmt_file , NULL);
  if (file_type == ecl_unified_summary_file)
    target_type = ecl_summary_file;
  else if (file_type == ecl_unified_restart_file)
    target_type = ecl_restart_file;
  else 
    util_exit("Can only unpack unified ECLIPSE summary and restart files\n");
  
  if (target_type == ecl_summary_file) {
    printf("** Warning: when unpacking unified summary files it as ambigous - starting with 0001  -> \n");
  }
  {
    fortio_type   * fortio_src = fortio_fopen( filename , "r" , true , fmt_file );
    ecl_file_type * src_file;
    int    offset;
    int    report_step = 0;
    char * path; 
    char * base;
    msg_type * msg;
    util_alloc_file_components( filename , &path , &base , NULL);
    {
      char * label  = util_alloc_sprintf("Unpacking %s => ", filename);
      msg = msg_alloc( label );
      free( label );
    }
    msg_show(msg);
    do {

      if (target_type == ecl_summary_file) {
	src_file = ecl_file_fread_alloc_summary_section( fortio_src );
	report_step += 1;
	offset = 0;
      } else {
	ecl_kw_type * seqnum_kw;
	src_file = ecl_file_fread_alloc_restart_section( fortio_src );
	if (src_file != NULL) {
	  seqnum_kw = ecl_file_iget_named_kw( src_file , "SEQNUM" , 0);
	  report_step = ecl_kw_iget_int( seqnum_kw , 0);
	}
	offset = 1;
      }
      
      if (src_file != NULL) {
	char * target_file = ecl_util_alloc_filename( path , base , target_type , fmt_file , report_step);
	fortio_type * fortio_target = fortio_fopen( target_file , "w" , true , fmt_file );
	
	msg_update(msg , target_file);
	ecl_file_fwrite_fortio( src_file , fortio_target , offset);

	fortio_fclose(fortio_target);
	free(target_file);
	ecl_file_free(src_file);
      }
    } while (src_file != NULL);
    fortio_fclose( fortio_src );
    util_safe_free(path);
    free(base);
    msg_free(msg , true);
  }
    

}


int main(int argc , char ** argv) {
  if (argc == 1)
    util_exit("ecl_unpack UNFIIED_FILE1   UNIFIED_FILE2   ...\n");
  {
    int iarg;
    for (iarg = 1; iarg < argc; iarg++)
      unpack_file( argv[iarg] );
  }
}
