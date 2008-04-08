
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
	  fprintf(stderr,"%s: failed to make directory:%s - aborting \n",__func__ , active_path);
	  fprintf(stderr,"%s \n",strerror(errno));
	  abort();
	}
      }

    } while (strlen(active_path) < strlen(_path));
    free(active_path);
  }
}


void util_make_path2(const char *path) {
  if (!util_path_exists(path)) {
    int length;
    int offset;
    if (path[0] == UTIL_PATH_SEP_CHAR)
      offset = 1;
    else
      offset = 0;
    length      = strcspn(&path[offset] , UTIL_PATH_SEP_STRING);
    {
      char * sub_path = util_alloc_substring_copy(&path[offset] , length);
      char * cwd      = NULL;
      cwd = getcwd(cwd , 0);
      if (mkdir(sub_path , 0775) != 0) {
	fprintf(stderr,"%s: failed to make directory:%s (ERROR:%d) - aborting \n",__func__ , sub_path, errno);
	fprintf(stderr,"%s \n",strerror(errno));
	abort();
      }
      chdir(sub_path);
      free(sub_path);
      util_make_path2(&path[offset + length + 1]);
      chdir(cwd);
    }
  }
}


/*
  path/prefix-pid-xxxxx
*/

char * util_alloc_tmp_file(const char * path, const char * prefix , bool include_pid ) {
  const int pid_digits    = 6;
  const int pid_max       = 1000000;
  const int random_digits = 6;
  const int random_max    = 1000000;

  
  pid_t  pid            = getpid() % pid_max;
  char * file           = malloc(strlen(path) + 1 + strlen(prefix) + 1 + pid_digits + 1 + random_digits + 1);
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



void util_unlink_path(const char *path) {
  if (util_path_exists(path)) {
    const uid_t uid = getuid();
    char *cwd       = NULL;
    struct dirent *dentry;
    cwd  = getcwd(cwd , 0);
    if (chdir(path) != 0) {
      fprintf(stderr,"%s: failed to change to %s - aborting \n", __func__ , path);
      abort();
    } else {
      DIR *dirH;
      char * new_cwd = getcwd(new_cwd , 0);
      dirH = opendir( new_cwd );  /* Have already changed into this directory with chdir() */
      while ( (dentry = readdir(dirH)) != NULL) {
	struct stat buffer;
	const char * entry = dentry->d_name;
	mode_t mode;
	if (lstat(entry , &buffer) != 0) {
	  fprintf(stderr,"%s: failed to stat(%s/%s) - aborting \n",__func__ , cwd , entry);
	  abort();
	} else {
	  mode = buffer.st_mode;

	  if (S_ISDIR(mode)) {
	    if ((strcmp(entry , ".") != 0) && (strcmp(entry , "..") != 0)) 
	      util_unlink_path(entry);
	  } else if (S_ISLNK(mode)) 
	    /*
	      Symbolic links are unconditionally removed.
	    */
	    unlink(entry);
	  else if (S_ISREG(mode)) {
	    if (buffer.st_uid == uid) 
	      unlink(entry);
	    else 
	      fprintf(stderr,"Warning mismatch in uid of calling process and entry owner for:%s - entry *not* removed \n",entry);
	  }
	}
      }
      free(new_cwd);
      closedir(dirH);
      chdir(cwd);
      free(cwd);

      if (rmdir(path) != 0) 
	fprintf(stderr,"%s: Warning: failed to remove director:%s \n",__func__ , path);
    }
  }
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
