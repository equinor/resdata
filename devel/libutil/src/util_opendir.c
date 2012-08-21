/**
   Externals:
*/
typedef struct msg_struct msg_type;
msg_type   * msg_alloc(const char * , bool);
void         msg_show(msg_type * );
void         msg_free(msg_type *  , bool);
void         msg_update(msg_type * , const char * );

static void util_copy_directory__(const char * src_path , const char * target_path , int buffer_size , void * buffer , msg_type * msg) {
  if (!util_is_directory(src_path))
    util_abort("%s: %s is not a directory \n",__func__ , src_path);
  
  util_make_path(target_path);
  {
    DIR * dirH = opendir( src_path );
    if (dirH == NULL) 
      util_abort("%s: failed to open directory:%s / %s \n",__func__ , src_path , strerror(errno));

    {
      struct dirent * dp;
      do {
        dp = readdir(dirH);
        if (dp != NULL) {
          if (dp->d_name[0] != '.') {
            char * full_src_path    = util_alloc_filename(src_path , dp->d_name , NULL);
            char * full_target_path = util_alloc_filename(target_path , dp->d_name , NULL);
            if (util_is_file( full_src_path )) {
              if (msg != NULL)
                msg_update( msg , full_src_path);
              util_copy_file__( full_src_path , full_target_path , buffer_size , buffer , true);
            } else 
              util_copy_directory__( full_src_path , full_target_path , buffer_size , buffer , msg);

            free( full_src_path );
            free( full_target_path );
          }
        }
      } while (dp != NULL);
    }
    closedir( dirH );
  }
}


/** 
    Equivalent to shell command cp -r src_path target_path
*/

/*  Does not handle symlinks (I think ...). */


void util_copy_directory(const char * src_path , const char * __target_path , const char * prompt) {
  int     num_components;
  char ** path_parts;
  char  * path_tail;
  char  * target_path;
  void * buffer   = NULL;
  int buffer_size = 512 * 1024 * 1024; /* 512 MB */
  do {
    buffer = malloc(buffer_size);
    if (buffer == NULL) buffer_size /= 2;
  } while ((buffer == NULL) && (buffer_size > 0));
  
  if (buffer_size == 0)
    util_abort("%s: failed to allocate any memory ?? \n",__func__);

  util_path_split(src_path , &num_components , &path_parts);
  path_tail   = path_parts[num_components - 1];
  target_path = util_alloc_filename(__target_path , path_tail , NULL);

  {
    msg_type * msg = NULL;
    if (prompt != NULL) {
      msg = msg_alloc(prompt , false);
      msg_show( msg );
      util_copy_directory__(src_path , target_path , buffer_size , buffer ,msg);
    }
    msg_free(msg , true);
  }
  free( buffer );
  free(target_path);
  util_free_stringlist( path_parts , num_components );
}
