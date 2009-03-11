#include <util.h>
#include <ecl_file.h>
#include <ecl_util.h>
#include <stdlib.h>
#include <msg.h>
   



int main(int argc, char ** argv) {
  int num_files = argc - 1;
  if (num_files >= 1) {
    /* File type and formatted / unformatted is determined from the first argument on the command line. */
    char * ecl_base;
    char * path;
    ecl_file_enum file_type , target_type;
    bool fmt_file;
    ecl_util_get_file_type( argv[1] , &file_type , &fmt_file , NULL);

    if (file_type == ecl_summary_file)
      target_type = ecl_unified_summary_file;
    else if (file_type == ecl_restart_file)
      target_type = ecl_unified_restart_file;
    else {
      fprintf(stderr , "The ecl_pack program can only be used with ECLIPSE restart files or summary files.\n");
      exit(1);
    }
    
    util_alloc_file_components( argv[1] , &path , &ecl_base , NULL);
    {
      msg_type * msg;
      int i , report_step , prev_report_step;
      char *  target_file_name = ecl_util_alloc_filename( path , ecl_base , target_type , fmt_file , -1);
      char ** filelist         = util_alloc_stringlist_copy( (const char **) &argv[1] , num_files );
      ecl_kw_type * seqnum_kw  = NULL;
      fortio_type * target     = fortio_fopen( target_file_name , "w" , true , fmt_file );

      if (target_type == ecl_unified_restart_file) {
	int dummy;
	seqnum_kw = ecl_kw_alloc_complete("SEQNUM" , 1 , ecl_int_type , &dummy);
      } 
      
      {
	char * msg_format = util_alloc_sprintf("Packing %s <= " , target_file_name);
	msg = msg_alloc( msg_format );
	free( msg_format );
      }
      
      msg_show( msg );
      qsort(filelist , num_files , sizeof *filelist , &ecl_util_fname_cmp);
      prev_report_step = -1;
      for (i=0; i < num_files; i++) {
	ecl_file_enum this_file_type;
	ecl_util_get_file_type(  filelist[i] , &this_file_type , NULL , &report_step);
	if (this_file_type == file_type) {
	  if (report_step == prev_report_step)
	    util_exit("Tried to write same report step twice: %s / %s \n",filelist[i-1] , filelist[i]);
	  prev_report_step = report_step;
	  msg_update(msg , filelist[i]);
	  {
	    ecl_file_type * src_file = ecl_file_fread_alloc( filelist[i] , true );
	    if (target_type == ecl_unified_restart_file) {
	      /* Must insert the SEQNUM keyword first. */
	      ecl_kw_iset_int(seqnum_kw , 0 , report_step);
	      ecl_kw_fwrite( seqnum_kw , target );
	    }
	    ecl_file_fwrite_fortio( src_file , target , 0);
	    ecl_file_free( src_file );
	  }
	}  /* Else skipping file of incorrect type. */
      }
      msg_free(msg , false);
      fortio_fclose( target );
      free(target_file_name);
      util_free_stringlist( filelist , num_files );
      if (seqnum_kw != NULL) ecl_kw_free(seqnum_kw);
    }
    free(ecl_base);
    util_safe_free(path);
  }
}
  
