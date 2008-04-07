#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>

#include <fortio.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_fstate.h>
#include <util.h>
#include <time.h>
#include <ecl_util.h>
#include <msg.h>

/*
  summary files: param[0..4]      = {TIME , YEARS , DAY , MONTH , YEAR}.
  smspec       : startdat[0..2]   = {day , month , year}
  restart      : intehead[64..66] = {day , month , year}
*/

  

struct ecl_fstate_struct {
  char 	      	 **filelist;
  bool 	      	   fmt_file;
  int              fmt_mode;
  bool 	      	   endian_convert;
  bool        	   unified;
  bool             __RPTONLY;
  int         	   files;
  int              N_blocks;
  int              block_size;
  ecl_block_type **block_list;
  time_t           sim_start_time;
  ecl_file_type    file_type;
};



bool ecl_fstate_fmt_file(const char *filename) {
  /*const int min_size = 32768;*/
  const int min_size = 1024;
  
  int report_nr;
  ecl_file_type file_type;

  bool fmt_file;
  if (util_file_exists(filename)) {
    if (util_file_size(filename) > min_size)
      fmt_file = util_fmt_bit8(filename);
    else {
      ecl_util_get_file_type(filename , &file_type , &fmt_file , &report_nr);
      if (file_type == ecl_other_file) {
	fprintf(stderr,"%s: sorry could not determine file type of file:%s - aborting \n",__func__ , filename);
	abort();
      }
    }
  } else {
    ecl_util_get_file_type(filename , &file_type , &fmt_file , &report_nr);
    if (file_type == ecl_other_file) {
      fprintf(stderr,"%s: sorry could not determine file type of file:%s - aborting \n",__func__ , filename);
      abort();
    }
  }
  
  return fmt_file;
}




ecl_fstate_type * ecl_fstate_alloc_empty(int fmt_mode , ecl_file_type file_type , bool endian_convert) {
  
  ecl_fstate_type *ecl_fstate 	 = malloc(sizeof *ecl_fstate);
  ecl_fstate->fmt_mode 	      	 = fmt_mode;
  ecl_fstate->endian_convert  	 = endian_convert;
  ecl_fstate->N_blocks        	 = 0;
  ecl_fstate->filelist        	 = NULL;
  ecl_fstate->block_list      	 = NULL;
  ecl_fstate->sim_start_time     = -1;
  ecl_fstate->file_type          = file_type;  
  ecl_fstate->__RPTONLY = false;
  return ecl_fstate;
}



static void __ecl_fstate_set_fmt(ecl_fstate_type *ecl_fstate) {

  switch(ecl_fstate->fmt_mode) {
  case ECL_FORMATTED:
    ecl_fstate->fmt_file = true;
    break;
  case ECL_BINARY:
    ecl_fstate->fmt_file = false;
    break;
  case ECL_FMT_AUTO:
    ecl_fstate->fmt_file = ecl_fstate_fmt_file(ecl_fstate->filelist[0]);
    break;
  default:
    fprintf(stderr,"%s: internal error - fmt_mode=%d invalid - aborting \n",__func__ , ecl_fstate->fmt_mode);
    abort();
  }
  
  {
    int i;
    for (i=0; i < ecl_fstate->N_blocks; i++)
      ecl_block_set_fmt_file(ecl_fstate->block_list[i] , ecl_fstate->fmt_file);
  }
}


bool ecl_fstate_set_fmt_mode(ecl_fstate_type *ecl_fstate , int fmt_mode) {
  ecl_fstate->fmt_mode = fmt_mode;
  __ecl_fstate_set_fmt(ecl_fstate);
  return ecl_fstate->fmt_mode;
}


void ecl_fstate_add_block(ecl_fstate_type *ecl_fstate , const ecl_block_type *new_block) {
  if (ecl_fstate->N_blocks == ecl_fstate->block_size) {
    ecl_fstate->block_size *= 2;
    ecl_fstate->block_list  = realloc(ecl_fstate->block_list , ecl_fstate->block_size * sizeof *ecl_fstate->block_list);
  }
  ecl_fstate->block_list[ecl_fstate->N_blocks] = (ecl_block_type *) new_block;
  ecl_fstate->N_blocks++;  
}


void ecl_fstate_set_files(ecl_fstate_type *ecl_fstate , int files , const char ** filelist) {
  ecl_fstate->files    = files;
  ecl_fstate->filelist = calloc(files , sizeof *ecl_fstate->filelist);
  ecl_fstate->unified  = ecl_util_unified(ecl_fstate->file_type); 
  if (ecl_fstate->unified) 
    ecl_fstate->filelist[0] = util_alloc_string_copy(filelist[0]);
  else {
    int file;
    for (file=0; file < files; file++) 
      ecl_fstate->filelist[file] = util_alloc_string_copy(filelist[file]);
  }
  __ecl_fstate_set_fmt(ecl_fstate);
}



void ecl_fstate_set_unified(ecl_fstate_type *ecl_fstate , bool unified) {
  ecl_fstate->unified = unified;
}





ecl_fstate_type * ecl_fstate_fread_alloc(int files , const char ** filelist , ecl_file_type file_type , bool endian_convert) {
  ecl_fstate_type *ecl_fstate = ecl_fstate_alloc_empty(ECL_FMT_AUTO , file_type , endian_convert);
  ecl_fstate_set_files(ecl_fstate , files , filelist);
  
  ecl_fstate->block_size  = 10;
  ecl_fstate->block_list  = calloc(ecl_fstate->block_size , sizeof *ecl_fstate->block_list);
  

  if (ecl_fstate->unified) {
    fortio_type *fortio = fortio_open(ecl_fstate->filelist[0] , "r" , ecl_fstate->endian_convert);
    int  summary_report_nr = 0;
    bool at_eof = false;
    while (!at_eof) {
      ecl_block_type *ecl_block = ecl_block_alloc(-1 , ecl_fstate->fmt_file , ecl_fstate->endian_convert);
      ecl_block_fread(ecl_block , fortio , &at_eof);
      
      if (file_type == ecl_unified_restart_file) {
	int report_nr;
	ecl_kw_type * seq_kw = ecl_block_get_kw(ecl_block , "SEQNUM");
	ecl_kw_iget(seq_kw , 0 , &report_nr);
	ecl_block_set_report_nr(ecl_block , report_nr);
	ecl_block_set_sim_time_restart(ecl_block);
      } else if (file_type == ecl_unified_summary_file) 
	if (ecl_fstate->__RPTONLY)
	  ecl_block_set_report_nr(ecl_block , summary_report_nr);

      /*
	Observe that when unified summary files are read in it is in
	general impossible to get hold of the report nr. **IF** the
	rptonly keyword has been present in the datafile (which we can
	only hope). It will be correct to just count blocks; that is
	what is done, but it is impossible to verify the correctness
	of this approach.
      */
      
      ecl_fstate_add_block(ecl_fstate , ecl_block);
      summary_report_nr++;
    }
    fortio_close(fortio);
  } else {    
    ecl_fstate->files = files;
    {
      int file;
      for (file=0; file < files; file++) {
	bool at_eof = false;
	fortio_type *fortio = fortio_open(ecl_fstate->filelist[file] , "r" , ecl_fstate->endian_convert);
	int report_nr = -1;
	
	if (file_type == ecl_restart_file || file_type == ecl_summary_file)
	  report_nr = ecl_util_filename_report_nr(ecl_fstate->filelist[file]);
	
	while (!at_eof) {
	  bool add_block = true;
	  ecl_block_type *ecl_block = ecl_block_alloc(report_nr , ecl_fstate->fmt_file , ecl_fstate->endian_convert);
	  ecl_block_fread(ecl_block , fortio , &at_eof );

	  if (file_type == ecl_restart_file)
	    ecl_block_set_sim_time_restart(ecl_block);

	  /*
	    In the case of summary files we can find incomplete files
	    with only the SEQHDR keyword; they are not added to the
	    fstate object.
	  */
	  if (file_type == ecl_summary_file) {
	    if (!ecl_block_has_kw(ecl_block , "MINISTEP")) 
	      add_block = false;
	  } 

	  ecl_block_set_report_nr(ecl_block , report_nr);
	  if (add_block)
	    ecl_fstate_add_block(ecl_fstate , ecl_block);
	  
	  if (file_type == ecl_summary_file) {
	    if (report_nr == 0) 
	      report_nr = 1;
	    else {
	      if (!at_eof) {
		if (ecl_fstate->__RPTONLY) {
		  fprintf(stderr,"\n%s: Several timesteps in summary file:%s allocated with summary_report_only = true - aborting.\n",__func__ , ecl_fstate->filelist[file]);
		  fprintf(stderr,"%s: Maybe you have forgot the keyword: \'RPTONLY\' in the datafile?\n",__func__);
		  abort();
		}
	      }
	    }
	  }
	}
	fortio_close(fortio);
      }
    }
  }
  return ecl_fstate;
}


/**
When you call this routine you *PROMISE* that the RPTONLY keyword has
been used in the ECLIPSE datafile, this is impossible to check, but if
called incorrectly (i.e. without having RPTONLY in the restart file),
temporal indexing of summary data will give wrong data - WITHOUT ANY
WARNING.
*/


void ecl_fstate_promise_RPTONLY(ecl_fstate_type * fstate) {
  fstate->__RPTONLY = true;
}


/** 
   The blocks in a fstat object can be indexed in two different ways:

   block_index: That is just the index of the block when it has been
                loaded. This method can always be used, but observe
                that there is no link between 'true' simulation time
                and the block index. To get a block with this method
                you should use ecl_fstate_iget_block().
		
   
   report_step: This is the report step from eclipse. It can always be
                used on restart files, never on 'other' files, and for
                summary files it can be used when the variable
                summary_report_only is true. This must be ensured with
                a call to ecl_fstate_summary_promise_rptonly().
*/


ecl_block_type * ecl_fstate_get_block_by_report_nr(const ecl_fstate_type * ecl_fstate , int report_nr) {
  if (ecl_fstate->file_type == ecl_unified_summary_file || ecl_fstate->file_type == ecl_summary_file) {
    if (!ecl_fstate->__RPTONLY) 
      util_abort("%s: when using report_nr to look up summary block you *MUST* have RPTONLY in the DATA file, and call ecl_fstate_promise_RPTONLY() - aborting \n",__func__);
  } else if (!(ecl_fstate->file_type == ecl_restart_file || ecl_fstate->file_type == ecl_unified_restart_file))
    util_abort("%s: can only use report_nr to look up restart & summary files - aborting.\n",__func__);

  {
    ecl_block_type *block = NULL;
    int block_index = 0;
    do {
      if (report_nr == ecl_block_get_report_nr(ecl_fstate->block_list[block_index]))
	block = ecl_fstate->block_list[block_index];
      block_index++;
    } while (block_index < ecl_fstate->N_blocks && block == NULL);
    
    if (block == NULL) 
      util_abort("%s: could not find report nr:%d - aborting \n",__func__ , report_nr);
    
    return block;
  }
}


ecl_block_type * ecl_fstate_iget_block(const ecl_fstate_type * ecl_fstate , int index) {
  int block_index = index;
  if (block_index < 0 || block_index >= ecl_fstate->N_blocks) {
    fprintf(stderr,"%s: index:%d invalid - legal range: [0,%d> - aborting \n",__func__ , index , ecl_fstate->N_blocks);
    abort();
  }
  return ecl_fstate->block_list[block_index];
}



bool ecl_fstate_has_report_nr(const ecl_fstate_type * ecl_fstate , int report_nr) {
  ecl_block_type * block = ecl_fstate_get_block_by_report_nr(ecl_fstate , report_nr);
  if (block == NULL) 
    return false;
  else
    return true;
}


int ecl_fstate_get_size(const ecl_fstate_type *ecl_fstate) {
  return ecl_fstate->N_blocks;
}



void ecl_fstate_free(ecl_fstate_type *ecl_fstate) {
  int i;
  for (i=0; i <ecl_fstate->files; i++)
    free(ecl_fstate->filelist[i]);
  free(ecl_fstate->filelist);

  for (i=0; i <ecl_fstate->N_blocks; i++)
    ecl_block_free(ecl_fstate->block_list[i]);
  free(ecl_fstate->block_list);

  free(ecl_fstate);
}


static void ecl_fstate_save_multiple(const ecl_fstate_type *ecl_fstate) {
  int block;
  for (block = 0; block < ecl_fstate->N_blocks; block++) {
    fortio_type *fortio = fortio_open(ecl_fstate->filelist[block] , "w" , ecl_fstate->endian_convert);
    ecl_block_fwrite(ecl_fstate->block_list[block] , fortio);
    fortio_close(fortio);
  }
}

static void ecl_fstate_save_unified(const ecl_fstate_type *ecl_fstate) {
  int block;
  fortio_type *fortio = fortio_open(ecl_fstate->filelist[0] , "w" , ecl_fstate->endian_convert);
  for (block = 0; block < ecl_fstate->N_blocks; block++) 
    ecl_block_fwrite(ecl_fstate->block_list[block] , fortio);
  fortio_close(fortio);
}

void ecl_fstate_save(const ecl_fstate_type *ecl_fstate) {
  if (ecl_fstate->unified)
    ecl_fstate_save_unified(ecl_fstate);
  else
    ecl_fstate_save_multiple(ecl_fstate);
}



int ecl_fstate_get_report_size(const ecl_fstate_type * ecl_fstate , int * first_report_nr , int * last_report_nr) {
  if (ecl_fstate->__RPTONLY) {
    *first_report_nr = ecl_block_get_report_nr(ecl_fstate->block_list[0]);
    *last_report_nr  = ecl_block_get_report_nr(ecl_fstate->block_list[ecl_fstate->N_blocks - 1]);
  } else {
    *first_report_nr = 0;
    *last_report_nr  = ecl_fstate->N_blocks - 1;
  }
  return ecl_fstate->N_blocks;
}
  


/*****************************************************************/

/*****************************************************************/


/*
  This function is spesifically made for the following:

  o Read an INIT file and replace some of the keywords with updated
    values.
 
  o It does not work on a proper ecl_fstate object, and in particular 
    does not handle replacing a keyword appearing several times in a 
    decent way.
*/


void ecl_fstate_filter_file(const char * src_file , const char * target_file , const hash_type * kw_hash , bool endian_flip) {
  if (util_same_file(src_file , target_file)) {
    fprintf(stderr,"%s %s and %s are the same (physical) file - this is not supported - aborting \n",__func__ , src_file , target_file);
    abort();
  }
  {
    fortio_type * src    = fortio_open(src_file , "r" , endian_flip );
    fortio_type * target = fortio_open(target_file , "w" , endian_flip);
    bool fmt_file        = ecl_fstate_fmt_file(src_file);
    ecl_kw_type * ecl_kw = ecl_kw_alloc_empty(fmt_file , endian_flip);
    bool OK;
    
    do {
      OK = ecl_kw_fread_realloc(ecl_kw , src);
      if (OK) {
	char * strip_kw = util_alloc_strip_copy(ecl_kw_get_header_ref(ecl_kw));
	if (hash_has_key(kw_hash , strip_kw)) {
	  ecl_kw_type * new_kw = hash_get(kw_hash , strip_kw);
	  if (new_kw != NULL) 
	    ecl_kw_fwrite(new_kw , target);
	} else
	  ecl_kw_fwrite(ecl_kw , target);
	free(strip_kw);
      }
    } while (OK);
    ecl_kw_free(ecl_kw);
    fortio_close(src);
    fortio_close(target);
  }
}
