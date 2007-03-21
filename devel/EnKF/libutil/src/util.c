#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include <util.h>



bool util_intptr_2bool(const int *iptr) {
  if ( (*iptr) == 0)
    return false;
  else
    return true;
}


char * util_alloc_cstring(const char *fort_string , const int *strlen) {
  const char null_char = '\0';
  char *new_string = malloc(*strlen + 1);
  strncpy(new_string , fort_string , *strlen);
  new_string[*strlen] = null_char;
  return new_string;
}

char * util_alloc_substring_copy(const char *src , int N) {
  char *copy;
  if (N < strlen(src)) {
    copy = malloc(N + 1);
    strncpy(copy , src , N);
    copy[N] = 0;
  } else 
    copy = util_alloc_string_copy(src);
  return copy;
}



/******************************************************************/


bool util_file_exists(const char *filename) {
  FILE *stream = fopen(filename , "r");
  bool ex;
  if (stream == NULL) {
    if (errno == ENOENT)
      ex = false;
    else {
      fprintf(stderr,"file: %s exists but open failed - aborting \n",filename);
      abort();
    }
  } else {
    fclose(stream);
    ex = true;
  }
  return ex;
}


int util_file_size(const char *file) {
  struct stat buffer;
  int fildes;
  
  fildes = open(file , O_RDONLY);
  fstat(fildes, &buffer);
  close(fildes);
  
  return buffer.st_size;
}

void util_unlink_existing(const char *filename) {
  if (util_file_exists(filename))
    unlink(filename);
}




bool util_fmt_bit8(const char *filename , int buffer_size) {
  const int min_read = 1024;
  FILE *stream;
  const double bit8set_limit = 0.00001;
  double bit8set_fraction;
  int N_bit8set = 0;
  char *buffer;
  int elm_read,i;

  if (util_file_exists(filename)) {
    buffer = malloc(buffer_size);
    stream = fopen(filename , "r");
    elm_read = fread(buffer , 1 , buffer_size , stream);
    if (elm_read < min_read) {
      fprintf(stderr,"Error in %s: file:%s is too small to automatically determine formatted/unformatted status \n",__func__ , filename);
      abort();
    }
    for (i=0; i < elm_read; i++)
      N_bit8set += (buffer[i] & (1 << 7)) >> 7;
    
    fclose(stream);
    free(buffer);
    
    bit8set_fraction = 1.0 * N_bit8set / elm_read;
    if (bit8set_fraction < bit8set_limit) 
      return true;
    else 
    return false;
  } else {
    fprintf(stderr,"%s: could not find file: %s - aborting \n",__func__ , filename);
    abort();
  }
}


void util_make_path(const char *_path) {
  char *active_path;
  char *path = (char *) _path;
  int current_pos = 0;

  if (!util_file_exists(path)) {
    active_path = malloc(strlen(path) + 1);
    int i = 0;
    do {
      int n = strcspn(path , "/");
      if (n < strlen(path))
	n += 1;
      path += n;
      i++;
      strncpy(active_path , _path , n + current_pos); 
      active_path[n+current_pos] = '\0';
      current_pos += n;
      if (!util_file_exists(active_path))    
	mkdir(active_path , 0775);
    } while (strlen(active_path) < strlen(_path));
  }
}


const char * util_newest_file(const char *file1 , const char *file2) {
  struct stat b1, b2;
  int f1,f2;
  time_t t1,t2;

  f1 = open(file1 , O_RDONLY);
  fstat(f1, &b1);
  t1 = b1.st_mtime;
  close(f1);

  f2 = open(file2 , O_RDONLY);
  fstat(f2, &b2);
  t2 = b2.st_mtime;
  close(f2);

  if (difftime(t1 , t2) > 0)
    return file1;
  else
    return file2;
}


/*****************************************************************/

void util_set_strip_copy(char * copy , const char *src) {
  const char null_char  = '\0';
  const char space_char = ' ';
  int i = 0;
  while (src[i] != null_char && src[i] != space_char) {
    copy[i] = src[i];
    i++;
  }
  copy[i] = null_char;
}


char * util_alloc_strip_copy(const char *src) {
  char *tmp = malloc(strlen(src) + 1);
  util_set_strip_copy(tmp , src);
  tmp = realloc(tmp , strlen(tmp) + 1);
  return tmp;
}


char * util_alloc_string_copy(const char *src ) {
  if (src != NULL) {
    char *copy = calloc(strlen(src) + 1 , sizeof *copy);
    strcpy(copy , src);
    return copy;
  } else 
    return NULL;
}


char * util_alloc_full_path(const char *path , const char *file) {
  char *copy = malloc(strlen(path) + strlen(file) + 1);
  sprintf(copy , "%s/%s" , path , file);
  return copy;
}



void util_free_string_list(char **list , int N) {
  int i;
  for (i=0; i < N; i++)
    free(list[i]);
  free(list);
}


char ** util_alloc_string_list(int N, int len) {
  int i;
  char **list = calloc(N , sizeof *list);
  for (i=0; i < N; i++)
    list[i] = malloc(len);
  return list;
}

/*****************************************************************/

void util_abort(const char *func, const char *file, int line, const char *message) {
  fprintf(stderr,"%s (%s:%d) %s - aborting \n",func,file,line,message);
  abort();
}



/*****************************************************************/

void util_unlink_path(const char *path) {
  if (util_file_exists(path)) {
    const uid_t uid = getuid();
    struct dirent *dentry;
    DIR *dirH;
    
    dirH = opendir(path);
    while ( (dentry = readdir(dirH)) != NULL) {
      struct stat buffer;
      char *entry = malloc(strlen(dentry->d_name) + 1 + strlen(path) + 1);
      sprintf(entry , "%s/%s", path , dentry->d_name);
      
      {
	int fildes;
	mode_t mode;
	fildes = open(entry , O_RDONLY);
	if (fildes > 0) {
	  fstat(fildes , &buffer);
	  close(fildes);
	  mode = buffer.st_mode;
	  
	  if (S_ISDIR(mode)) {
	    if ((strcmp(dentry->d_name , ".") != 0) && (strcmp(dentry->d_name , "..") != 0)) 
	      util_unlink_path(entry);
	  } else if (S_ISREG(mode) || S_ISLNK(mode)) {
	    if (buffer.st_uid == uid) 
	      unlink(entry);
	    else 
	      fprintf(stderr,"Warning mismatch in uid of calling process and entry owner for:%s - entry *not* removed \n",entry);
	  } 
	}
      }
      free(entry);
    }
    closedir(dirH);
    if (rmdir(path) != 0) {
      fprintf(stderr," Failed to remove directory: %s \n",path);
      abort();
    }
  }
}


/*****************************************************************/


typedef struct {
  const char *filename;
  int         num_offset;
} filenr_type;



static int enkf_filenr(filenr_type filenr) {
  const char * num_string = &filenr.filename[filenr.num_offset];
  return strtol(num_string , NULL , 10);
}


static int enkf_filenr_cmp(const void *_file1 , const void *_file2) {
  const filenr_type  *file1 = (const filenr_type *) _file1;
  const filenr_type  *file2 = (const filenr_type *) _file2;
  
  int nr1 = enkf_filenr(*file1);
  int nr2 = enkf_filenr(*file2);
  
  if (nr1 < nr2)
    return -1;
  else if (nr1 > nr2)
    return 1;
  else
    return 0;
}


void util_enkf_unlink_ensfiles(const char *enspath , const char *ensbase, int mod_keep , bool dryrun) {
  DIR *dir_stream;
  

  if ( (dir_stream = opendir(enspath)) ) {
    const int len_ensbase = strlen(ensbase);
    const int num_offset  = len_ensbase + 1 + strlen(enspath);
    filenr_type *fileList;
    struct dirent *entry;

    int first_file = 999999;
    int last_file  = 0;
    int filenr;
    int files = 0;
    
    while ( (entry = readdir(dir_stream)) ) {
      if (strncmp(ensbase , entry->d_name , len_ensbase) == 0)
	files++;
    }
    if (files == 0) {
      closedir(dir_stream);
      fprintf(stderr,"%s: found no matching ensemble files \n",__func__);
      return;
    }
    
    fileList = malloc(files * sizeof *fileList);
    
    filenr = 0;
    rewinddir(dir_stream);
    while ( (entry = readdir(dir_stream)) ) {
      if (strncmp(ensbase , entry->d_name , len_ensbase) == 0) {
	fileList[filenr].filename   = util_alloc_full_path(enspath , entry->d_name);
	fileList[filenr].num_offset = num_offset;
	{
	  int num = enkf_filenr(fileList[filenr]);
	  if (num < first_file) first_file = num;
	  if (num > last_file)  last_file  = num;
	}
	filenr++;
      }
    } 
    closedir(dir_stream);
    qsort(fileList , files , sizeof *fileList , enkf_filenr_cmp);

    for (filenr = 0; filenr < files; filenr++) {
      int num = enkf_filenr(fileList[filenr]);
      bool delete_file = false;

      if (num != first_file && num != last_file) 
	if ( (num % mod_keep) != 0) 
	  delete_file = true;

      if (delete_file) {
	if (dryrun)
	  printf("    %s\n",fileList[filenr].filename);
	else {
	  printf("Deleting: %s \n",fileList[filenr].filename);
	  unlink(fileList[filenr].filename);
	}
      } 
    }
    for (filenr = 0; filenr < files; filenr++) 
      free((char *) fileList[filenr].filename);
    free(fileList);
  } else {
    fprintf(stderr,"%s: failed to open directory: %s - aborting \n",__func__ , enspath);
    abort();
  }
  
}





