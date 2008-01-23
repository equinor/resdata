#include <stdlib.h>
#include <util.h>
#include <string.h>



void util_print_string__(const char *_s,
			 const int  *s_len) {
  char * s = util_alloc_cstring(_s , s_len);
  printf("%s" , s);
  fflush(stdout);
  free(s);
}



void util_enkf_unlink_ensfiles__(const char *_enspath , const int *enspath_len , 
			    const char *_ensbase , const int *ensbase_len , 
			    const int *mod_keep, const int *dryrun_int) {
  char *enspath = util_alloc_cstring(_enspath , enspath_len);
  char *ensbase = util_alloc_cstring(_ensbase , ensbase_len);
  util_enkf_unlink_ensfiles(enspath , ensbase, *mod_keep , util_intptr_2bool(dryrun_int));
  free(enspath);
  free(ensbase);
}



void util_getcwd__(char *cwd , int *f90_length) {
  getcwd(cwd , (*f90_length) - 1);
  util_pad_f90string(cwd , strlen(cwd) , *f90_length);
}



void util_proc_alive__(const int *pid , int * alive) {
  if (util_proc_alive(*pid))
    *alive = 1;
  else
    *alive = 0;
}



void util_make_path__(const char * _path, const int *path_len) {
  char * path = util_alloc_cstring(_path , path_len);
  util_make_path(path);
  free(path);
}


void util_path_exists__(const char * _path , const int * path_len , int *result) {
  char * path = util_alloc_cstring(_path , path_len);
  if (util_path_exists(path))
    *result = 1;
  else
    *result = 0;
  free(path);
}


void util_read_path__(const char * _prompt, const int *prompt_len1, const int * prompt_len2 , const int * must_exist_int , char *path , const int * path_length) {
  char * prompt = util_alloc_cstring(_prompt , prompt_len1);
  util_read_path(prompt , *prompt_len2 , util_intptr_2bool(must_exist_int) , path);
  util_pad_f90string(path , strlen(path) , *path_length);
  free(prompt);
}

void util_read_filename__(const char * _prompt, const int *prompt_len1, const int * prompt_len2 , const int * must_exist_int , char *path , const int * path_length) {
  char * prompt = util_alloc_cstring(_prompt , prompt_len1);
  util_read_filename(prompt , *prompt_len2 , util_intptr_2bool(must_exist_int) , path);
  util_pad_f90string(path , strlen(path) , *path_length);
  free(prompt);
}


void util_read_string__(const char * _prompt, const int *prompt_len1, const int * prompt_len2 , char *string , const int * string_length) {
  char * prompt = util_alloc_cstring(_prompt , prompt_len1);
  util_read_string(prompt , *prompt_len2 , string);
  util_pad_f90string(string , strlen(string) ,  *string_length);
  free(prompt);
}


void util_make_path_interactive__(const char *_prompt , const int *prompt_len1 , const int *prompt_len2, char * path , int * f90_length) {
  char * prompt = util_alloc_cstring(_prompt , prompt_len1);
  util_read_path(prompt , *prompt_len2 , false , path);
  util_make_path(path);
  util_pad_f90string(path , strlen(path) , *f90_length);
  free(prompt);
}



void util_copy_file__(const char * _src_file , const int * src_file_len , const char * _target_file , const int * target_file_len) {
  char * src_file    = util_alloc_cstring(_src_file    , src_file_len);
  char * target_file = util_alloc_cstring(_target_file , target_file_len); 

  util_copy_file(src_file , target_file);

  free(src_file);
  free(target_file);
}



void util_make_enkf_run_path__(const char *_run_path_root , const int * run_path_root_len , const char *_run_path_link , const int * run_path_link_len , char * run_path, const int * run_path_len) {
  char * run_path_root = util_alloc_cstring(_run_path_root , run_path_root_len);
  char * run_path_link = util_alloc_cstring(_run_path_link , run_path_link_len);
  util_make_path(run_path_root);
  {
    const int cwd_buffer_size = 512;
    char *cwd_tail;
    char cwd[cwd_buffer_size];
    
    getcwd( cwd , cwd_buffer_size );
    cwd_tail = strrchr(cwd , '/');
    if (cwd_tail == NULL) 
      fprintf(stderr,"%s: Warning could not find '/' in cwd:%s - this seems peculiar.\n",__func__ , cwd);
    else {
      sprintf(run_path , "%s%s" , run_path_root , cwd_tail);
      util_make_path(run_path);
    }
    if (util_file_exists(run_path_link)) {
      if (util_is_link(run_path_link))
	util_unlink_existing(run_path_link);
      else {
	fprintf(stderr,"%s: %s already exists - and is not a symlink - aborting \n",__func__ , run_path_link);
	abort();
      }
    }
    util_make_slink(run_path , run_path_link);
    util_pad_f90string(run_path , strlen(run_path) , *run_path_len);
  }
  free(run_path_root);
}



void util_inter_file_slink__(const char *_target , const int * target_len , const char * _link , const int * link_len) {
  char * target = util_alloc_cstring(_target , target_len);
  char * link   = util_alloc_cstring(_link   , link_len);

  util_make_slink(target , link);
  
  free(target);
  free(link);
}

/*
void util_bool_test__(const int * f90_input) {
  printf("Har fått inn : %d \n", *f90_input);
}
*/
