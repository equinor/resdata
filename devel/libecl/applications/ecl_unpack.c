#include <stdbool.h>
#include <util.h>
#include <ecl_file.h>
#include <ecl_util.h>
#include <ecl_kw.h>
#include <msg.h>
#include <ecl_endian_flip.h>

void unpack_file(const char * filename) {
  ecl_file_enum target_type = ECL_OTHER_FILE;
  ecl_file_enum file_type;
  bool fmt_file;
  file_type = ecl_util_get_file_type(filename , &fmt_file , NULL);
  if (file_type == ECL_UNIFIED_SUMMARY_FILE)
    target_type = ECL_SUMMARY_FILE;
  else if (file_type == ECL_UNIFIED_RESTART_FILE)
    target_type = ECL_RESTART_FILE;
  else 
    util_exit("Can only unpack unified ECLIPSE summary and restart files\n");
  
  if (target_type == ECL_SUMMARY_FILE) {
    printf("** Warning: when unpacking unified summary files it as ambigous - starting with 0001  -> \n");
  }
  {
    fortio_type   * fortio_src = fortio_fopen( filename , "r" , ECL_ENDIAN_FLIP , fmt_file );
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

      if (target_type == ECL_SUMMARY_FILE) {
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


      /**
         Will unpack to cwd, even though the source files might be
         somewhere else. To unpack to the same directory as the source
         files, just send in @path as first argument when creating the
         target_file.
      */
      
      if (src_file != NULL) {
	char * target_file = ecl_util_alloc_filename( NULL , base , target_type , fmt_file , report_step);
	fortio_type * fortio_target = fortio_fopen( target_file , "w" , ECL_ENDIAN_FLIP , fmt_file );
	
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
    util_exit("ecl_unpack UNIFIED_FILE1   UNIFIED_FILE2   ...\n");
  {
    int iarg;
    for (iarg = 1; iarg < argc; iarg++)
      unpack_file( argv[iarg] );
  }
}
