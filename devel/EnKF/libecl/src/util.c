#include <errno.h>
#include <time.h>
#include <util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


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




bool util_fmt_bit8(const char *filename , int buffer_size) {
  const int min_read = 1024;
  FILE *stream;
  const double bit8set_limit = 0.00001;
  double bit8set_fraction;
  int N_bit8set = 0;
  char *buffer;
  int elm_read,i;


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

