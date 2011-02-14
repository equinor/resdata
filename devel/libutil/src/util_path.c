/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'util_path.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <stdlib.h>
/**
  This little function checks if the supplied path is an abolute path,
  or a relative path. The check is extremely simple - if the first
  character equals "/" (on Unix) it is interpreted as an abolute path, 
  otherwise not.
*/


bool util_is_abs_path(const char * path) {
  if (path[0] == UTIL_PATH_SEP_CHAR)
    return true;
  else
    return false;
}



static bool util_make_path2__(const char *path , mode_t mode) {
  /* It already exists as a directory - we just return true. */
  if (util_is_directory( path ))
    return true;      
  
  /* It is in the filesystem - but not as a directory - then we can do nothing. */
  if (util_entry_exists( path ))
    return false;
  
  if (mkdir( path , mode ) == 0)
    return true;
  else
    return false;
}



/**
   Will return if the directory path exists on exit, either because it
   already existed, or because it was successfully made with this
   function. 
   
   If a file entry 'path' already exists, and is NOT a directory, the
   function will return false.

   All directories which are actually created, are created with mode
   @mode - no attempt is made to change the mode of existing
   directories. 

   Observe that the mode is an 'absolute' mode; umask is explicitly
   circumvented when mkdir() is called.
*/



bool util_make_path2( const char * path , mode_t mode) {

  /* It already exists as a directory - we just return true. */
  if (util_is_directory( path ))
    return true;      
  
  /* It is in the filesystem - but not as a directory - then we can do nothing. */
  if (util_entry_exists( path ))
    return false;
  {
    
    bool    make_path = true;
    char ** components;
    int     num_components;
    int     len = 1;
    bool    abs_path = util_is_abs_path( path );
    mode_t  old_mode = umask( 0 );
    
    util_split_string( path , UTIL_PATH_SEP_STRING , &num_components , &components );
    while (make_path && (len  <= num_components)) {
      char * current_path = util_alloc_joined_string( (const char **) components , len , UTIL_PATH_SEP_STRING );

      /* Prepend the '/' for absolute path. */
      if (abs_path) {
        int length = strlen( current_path );
        current_path = util_realloc(current_path , length + 2 , __func__);
        memmove( &current_path[1] , current_path , length + 1);
        current_path[0] = UTIL_PATH_SEP_CHAR;
      }
      make_path = util_make_path2__( current_path , mode );
      len++;
      free( current_path );
    }

    umask( old_mode );
    return make_path;
  }
}



void util_make_path(const char *_path) {
  char *active_path;
  char *path = (char *) _path;
  int current_pos = 0;

  if (!util_is_directory(path)) {
    active_path = util_malloc(strlen(path) + 1 , __func__);
    int i = 0;
    do {
      int n = strcspn(path , UTIL_PATH_SEP_STRING);
      if (n < strlen(path))
	n += 1;
      path += n;
      i++;
      strncpy(active_path , _path , n + current_pos); 
      active_path[n+current_pos] = '\0';
      current_pos += n; 
      
      if (!util_is_directory(active_path)) {
	if (mkdir(active_path , UTIL_DEFAULT_MKDIR_MODE) != 0) { 
	  bool fail = false;
	  switch (errno) {
	  case(EEXIST):
	    if (util_is_directory(active_path))
	      fail = false;
	    break;
	  case(ENOSPC):
	    /* 
	       We try to handle "No space left on the device" by letting the user 
	       get a chance to clean out the disk.
	    */
	    __block_full_disk(active_path);
	    fail = false;
	    util_make_path(active_path);
	    break;
	  default:
	    fail = true;
	    break;
	  }
	  if (fail)
	    util_abort("%s: failed to make directory:%s - aborting\n: %s(%d) \n",__func__ , active_path , strerror(errno), errno);
	}
      }
      
    } while (strlen(active_path) < strlen(_path));
    free(active_path);
  }
}


/**
   This function will allocate a unique filename with a random part in
   it. If the the path corresponding to the first argument does not
   exist it is created.

   If the value include_pid is true, the pid of the calling process is
   included in the filename, the resulting filename will be:

      path/prefix-pid-RANDOM

   if include_pid is false the resulting file will be:

      path/prefix-RANDOM

   Observe that IFF the prefix contains any path separator character
   they are translterated to "_".
*/

char * util_alloc_tmp_file(const char * path, const char * prefix , bool include_pid ) {
  const int pid_digits    = 6;
  const int pid_max       = 1000000;
  const int random_digits = 6;
  const int random_max    = 1000000;

  
  pid_t  pid            = getpid() % pid_max;
  char * file           = util_malloc(strlen(path) + 1 + strlen(prefix) + 1 + pid_digits + 1 + random_digits + 1 , __func__);
  char * tmp_prefix     = util_alloc_string_copy( prefix );
  
  if (!util_is_directory(path))
    util_make_path(path);
  util_string_tr( tmp_prefix ,  UTIL_PATH_SEP_CHAR , '_');  /* removing path seps. */
  
  do {
    long int rand_int = random() % random_max;
    if (include_pid)
      sprintf(file , "%s%c%s-%d-%ld" , path , UTIL_PATH_SEP_CHAR , tmp_prefix , pid , rand_int);
    else
      sprintf(file , "%s%c%s-%ld" , path , UTIL_PATH_SEP_CHAR , tmp_prefix , rand_int);
  } while (util_file_exists(file));

  free( tmp_prefix );
  return file;
}


/**
   This file allocates a filename consisting of a leading path, a
   basename and an extension. Both the path and the extension can be
   NULL, but not the basename. 
   
   Observe that this function does pure string manipulation; there is
   no input check on whether path exists, if basaneme contains "."
   (or even a '/') and so on.
*/

char * util_alloc_filename(const char * path , const char * basename , const char * extension) {
  bool   include_path = false;
  char * file;
  int    length = strlen(basename) + 1; 
  
  if (path != NULL) {
    include_path = true;
    length += strlen(path) + 1;
  }
  if (extension != NULL)
    length += strlen(extension) + 1;

  file = util_malloc(length , __func__);

  if (path == NULL) {
    if (extension == NULL)
      memcpy(file , basename , strlen(basename) + 1);
    else
      sprintf(file , "%s.%s" , basename , extension);
  } else {
    if (extension == NULL)
      sprintf(file , "%s%c%s" , path , UTIL_PATH_SEP_CHAR , basename);
    else
      sprintf(file , "%s%c%s.%s" , path , UTIL_PATH_SEP_CHAR , basename , extension);
  }
  return file;
}


char * util_realloc_filename(char * filename , const char * path , const char * basename , const char * extension) {
  util_safe_free(filename);
  return util_alloc_filename( path , basename , extension );
}





/**
   Only removes the last component in path.
*/
void static util_clear_directory__( const char *path , bool strict_uid , bool unlink_root) {
  if (util_is_directory(path)) {
    DIR  *dirH = opendir( path );

    if (dirH != NULL) {
      const uid_t uid = getuid();
      struct dirent *dentry;
      
      while ( (dentry = readdir(dirH)) != NULL) {
        struct stat buffer;
        mode_t mode;
        const char * entry_name = dentry->d_name;
        if ((strcmp(entry_name , ".") != 0) && (strcmp(entry_name , "..") != 0)) {
          char * full_path = util_alloc_filename(path , entry_name , NULL);

          if (lstat(full_path , &buffer) == 0) {
            mode = buffer.st_mode;
	  
            if (S_ISDIR(mode)) 
              /*
                Recursively descending into sub directory. 
              */
              util_clear_directory__(full_path , strict_uid , true);
            else if (S_ISLNK(mode)) 
              /*
                Symbolic links are unconditionally removed.
              */
              unlink(full_path);
            else if (S_ISREG(mode)) {
              /* 
                 It is a regular file - we remove it (if we own it!).
              */
              if ((!strict_uid) || (buffer.st_uid == uid)) {
                int unlink_return = unlink(full_path);
                if (unlink_return != 0) {
                  /* Unlink failed - we don't give a shit. */
                }
              } 
            }
          }
          free(full_path);
        }
      }
    }
    closedir(dirH);

    /* Finish with clearing the root directory */
    if (unlink_root) {
      int rmdir_return = rmdir(path);
      if (rmdir_return != 0) {
        /* Unlink failed - we don't give a shit. */
      }
    }
  }
}


/**
   This function will clear away all the contents (including
   subdirectories) in the directory @path.

   If the parameter @strict_uid is set to true, the function will only
   attempt to remove entries where the calling uid is also the owner
   of the entry. 

   If the parameter @unlink_root is true the directory @path will also
   be removed, otherwise it will be left as an empty directory.

   The The function will just go about deleting as much as it can; errors
   are not signalled in any way!
*/


void util_clear_directory(const char * path , bool strict_uid , bool unlink_root) {
  util_clear_directory__( path , strict_uid , unlink_root );
}


bool util_proc_alive(pid_t pid) {
  char proc_path[16];
  sprintf(proc_path , "/proc/%d" , pid);
  return util_is_directory(proc_path);
}


int util_proc_mem_free(void) {
  FILE *stream = util_fopen("/proc/meminfo" , "r");
  int mem;
  util_fskip_lines(stream , 1);
  util_fskip_token(stream);
  util_fscanf_int(stream , &mem);
  fclose(stream);
  return mem;
}


char * util_alloc_realpath(const char * input_path) {
  char * buffer   = util_malloc(PATH_MAX + 1 , __func__);
  char * new_path = NULL;
  
  new_path = realpath( input_path , buffer);
  if (new_path == NULL) 
    util_abort("%s: input_path:%s - failed %s(%d) \n",__func__ , input_path , strerror(errno) , errno);
  else 
    new_path = util_realloc(new_path , strlen(new_path) + 1, __func__);
  
  return new_path;
}



bool util_try_alloc_realpath(const char * input_path) {
  char * buffer   = util_malloc(PATH_MAX + 1 , __func__);
  char * new_path = NULL;

  new_path = realpath( input_path , buffer);
  free(buffer);
  if (new_path == NULL) 
    return false;
  else 
    return true;
}




void util_path_split(const char *line , int *_tokens, char ***_token_list) {
  util_split_string( line , UTIL_PATH_SEP_STRING , _tokens , _token_list);
}
