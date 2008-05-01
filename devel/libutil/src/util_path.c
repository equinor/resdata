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


int util_get_path_length(const char * file) {
  if (util_is_directory(file)) 
    return strlen(file);
  else {
    char * last_slash = strrchr(file , UTIL_PATH_SEP_CHAR);
    if (last_slash == NULL)
      return 0;
    else 
      return last_slash - file;
  }
}


void util_make_path(const char *_path) {
  char *active_path;
  char *path = (char *) _path;
  int current_pos = 0;

  if (!util_path_exists(path)) {
    active_path = malloc(strlen(path) + 1);
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
      
      if (!util_path_exists(active_path)) {
	if (mkdir(active_path , 0775) != 0) { 
	  if (!((errno == EEXIST) && util_is_directory(active_path)))   /* errnoe == EEXIST another thread might have made it for us.... */
	    util_abort("%s: failed to make directory:%s - aborting.\n %s \n",__func__ , active_path , strerror(errno));
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

   if include_pi is false the resulting file will be:

      path/prefix-RANDOM

   Observe that prefix can *NOT* contain any path sections, i.e. '/'.
*/

char * util_alloc_tmp_file(const char * path, const char * prefix , bool include_pid ) {
  const int pid_digits    = 6;
  const int pid_max       = 1000000;
  const int random_digits = 6;
  const int random_max    = 1000000;

  
  pid_t  pid            = getpid() % pid_max;
  char * file           = util_malloc(strlen(path) + 1 + strlen(prefix) + 1 + pid_digits + 1 + random_digits + 1 , __func__);
  
  if (!util_is_directory(path))
    util_make_path(path);
  {
    int i;
    for (i = 0; i < strlen(prefix); i++)
      if (prefix[i] == UTIL_PATH_SEP_CHAR)
	util_abort("%s: prefix:%s invalid - can not contain path separator:\'%s\' - aborting\n",__func__ , prefix , UTIL_PATH_SEP_STRING);
  }

  
  do {
    long int rand_int = random() % random_max;
    if (include_pid)
      sprintf(file , "%s%c%s-%d-%ld" , path , UTIL_PATH_SEP_CHAR , prefix , pid , rand_int);
    else
      sprintf(file , "%s%c%s-%ld" , path , UTIL_PATH_SEP_CHAR , prefix , rand_int);
  } while (util_file_exists(file));
  return file;

}




/**
  This function takes a path and a file as input. It allocated a new
  string containg "the sum" of the two, with UTIL_PATH_SEP between.

  If path == NULL - a copy of file is returned.
*/


char * util_alloc_full_path(const char *path , const char *file) {
  if (path != NULL) {
    char *copy = util_malloc(strlen(path) + strlen(file) + 2 , __func__);
    sprintf(copy , "%s%c%s" , path , UTIL_PATH_SEP_CHAR ,  file);
    return copy;
  } else
    return util_alloc_string_copy(file);
}


char * util_realloc_full_path(char *old_path , const char *path , const char *file) {
  char *copy = realloc(old_path , strlen(path) + strlen(file) + 2);
  sprintf(copy , "%s%c%s" , path , UTIL_PATH_SEP_CHAR , file);
  return copy;
}



void static util_unlink_path_static(const char *path , bool test_mode) {
  if (util_path_exists(path)) {
    const uid_t uid = getuid();
    DIR  *dirH;
    struct dirent *dentry;
    dirH = opendir( path );  

    while ( (dentry = readdir(dirH)) != NULL) {
      struct stat buffer;
      mode_t mode;
      const char * entry_name = dentry->d_name;
      if ((strcmp(entry_name , ".") != 0) && (strcmp(entry_name , "..") != 0)) {
	char * full_path = util_alloc_full_path(path , entry_name);

	if (lstat(full_path , &buffer) != 0) {
	  fprintf(stderr,"%s: failed to stat: %s entry not removed.\n",__func__ , full_path);
	} else {
	  mode = buffer.st_mode;
	  
	  if (S_ISDIR(mode)) {
	    util_unlink_path_static(full_path, test_mode);
	  } else if (S_ISLNK(mode)) {
	    /*
	      Symbolic links are unconditionally removed.
	    */
	    if (test_mode)
	      printf("%s [TEST:] removing symbolic link: %s \n",__func__ , full_path);
	    else
	      unlink(full_path);
	  }
	  else if (S_ISREG(mode)) {
	    if (buffer.st_uid == uid) {
	      if (test_mode)
		printf("%s [TEST:] removing file: %s \n",__func__ , full_path);
	      else {
		if (unlink(full_path) != 0) {
		  fprintf(stderr,"%s: failed to unlink: %s ?? \n",__func__ , full_path);
		  fprintf(stderr,"%s: %s(%d) \n",__func__ , strerror(errno) , errno);
		}
	      }
	    } else 
	      fprintf(stderr,"Warning mismatch in uid of calling process and owner for:%s - entry *not* removed \n",full_path);
	  }
	}
	free(full_path);
      }
    }
    
    closedir(dirH);
    if (test_mode)
      printf("%s: [TEST:] removing directory: %s \n",__func__ , path);
    else {
      int rmdir_return = rmdir(path);
      /*
	The "Directory not empty" warning was displayed *all the time*
	on NFS mounted volumes - some .nfsxxxxxx file was hanging
	around - so this warning is masked out.
      */
      
      if ((rmdir_return != 0) && (rmdir_return != ENOTEMPTY)) 
	fprintf(stderr,"%s: Warning: failed to remove directory:%s: %s \n",__func__ , path , strerror(errno));
    }
  }
}



void util_unlink_path(const char *path) {
  util_unlink_path_static(path , false);
}

void util_unlink_path_TESTING(const char *path) {
  util_unlink_path_static(path , true);
}



bool util_proc_alive(pid_t pid) {
  char proc_path[16];
  sprintf(proc_path , "/proc/%d" , pid);
  return util_path_exists(proc_path);
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
