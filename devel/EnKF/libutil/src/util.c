#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <util.h>


#define FLIP16(var) (((var >> 8) & 0x00ff) | ((var << 8) & 0xff00))

#define FLIP32(var) (( (var >> 24) & 0x000000ff) | \
		      ((var >>  8) & 0x0000ff00) | \
		      ((var <<  8) & 0x00ff0000) | \
		      ((var << 24) & 0xff000000))

#define FLIP64(var) (((var >> 56)  & 0x00000000000000ff) | \
		      ((var >> 40) & 0x000000000000ff00) | \
		      ((var >> 24) & 0x0000000000ff0000) | \
		      ((var >>  8) & 0x00000000ff000000) | \
		      ((var <<  8) & 0x000000ff00000000) | \
		      ((var << 24) & 0x0000ff0000000000) | \
		      ((var << 40) & 0x00ff000000000000) | \
		      ((var << 56) & 0xff00000000000000))


/*****************************************************************/



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


char * util_alloc_dequoted_string(char *s) {
  char *new;
  int offset,len;
  if (s[0] == '\'')
    offset = 1;
  else 
    offset = 0;
  
  if (s[strlen(s)] == '\'')
    len = strlen(s) - 1 - offset;
  else
    len = strlen(s) - offset;
  
  new = util_alloc_substring_copy(&s[offset] , len);
  free(s);
  return new;
}




/******************************************************************/

void util_copy_stream(FILE *src_stream , FILE *target_stream , int buffer_size , void * buffer) {

  while ( ! feof(src_stream)) {
    int bytes;
    bytes = fread (buffer , 1 , bytes , src_stream);
    fwrite(buffer , 1 , bytes , target_stream);
  }

}


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


bool util_path_exists(const char *pathname) {
  DIR *stream = opendir(pathname);
  bool ex;
  if (stream == NULL) {
    ex = false;
  } else {
    closedir(stream);
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

  if (!util_path_exists(path)) {
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
      if (!util_path_exists(active_path))    
	mkdir(active_path , 0775);
    } while (strlen(active_path) < strlen(_path));
  }
}



double util_file_difftime(const char *file1 , const char *file2) {
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

  return difftime(t1 , t2);
}

const char * util_newest_file(const char *file1 , const char *file2) {
  if (util_file_difftime(file1 , file2) < 0)
    return file1;
  else
    return file2;
}


bool util_file_update_required(const char *src_file , const char *target_file) {
  if (util_file_difftime(src_file , target_file) < 0)
    return true;
  else
    return false;
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


char * util_realloc_string_copy(char * old_string , const char *src ) {
  if (src != NULL) {
    char *copy = realloc(old_string , (strlen(src) + 1) * sizeof *copy);
    strcpy(copy , src);
    return copy;
  } else 
    return NULL;
}

char * util_realloc_substring_copy(char * old_string , const char *src , int len) {
  if (src != NULL) {
    int str_len;
    char *copy;
    if (strlen(src) < len)
      str_len = strlen(src);
    else
      str_len = len;

    copy = realloc(old_string , (str_len + 1) * sizeof *copy);
    strncpy(copy , src , str_len);
    copy[str_len] = '\0';

    return copy;
  } else 
    return NULL;
}


char * util_alloc_full_path(const char *path , const char *file) {
  char *copy = malloc(strlen(path) + strlen(file) + 2);
  sprintf(copy , "%s/%s" , path , file);
  return copy;
}


char * util_realloc_full_path(char *old_path , const char *path , const char *file) {
  char *copy = realloc(old_path , strlen(path) + strlen(file) + 2);
  sprintf(copy , "%s/%s" , path , file);
  return copy;
}



void util_free_string_list(char **list , int N) {
  int i;
  for (i=0; i < N; i++) {
    if (list[i] != NULL)
      free(list[i]);
  }
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





/*****************************************************************/



void util_unlink_path(const char *path) {
  if (util_path_exists(path)) {
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

/*****************************************************************/

void util_split_string(const char *line , const char *sep, int *_tokens, char ***_token_list) {
  int offset;
  int tokens , token , token_length;
  char **token_list;
  
  offset = strspn(line , sep);
  tokens = 0;
  do {
    token_length = strcspn(&line[offset] , sep);
    if (token_length > 0)
      tokens++;
    offset += token_length;
  } while (line[offset] != '\0');
  token_list = malloc(tokens * sizeof * token_list);
  
  offset = strspn(line , sep);
  token  = 0;
  do {
    token_length = strcspn(&line[offset] , sep);
    if (token_length > 0) {
      token_list[token] = util_alloc_substring_copy(&line[offset] , token_length);
      token++;
    } else
      token_list[token] = NULL;
    
    offset += token_length;
  } while (line[offset] != '\0');
  
  *_tokens     = tokens;
  *_token_list = token_list;
}


/*****************************************************************/

void util_float_to_double(double *double_ptr , const float *float_ptr , int size) {
  int i;
  for (i=0; i < size; i++)
    double_ptr[i] = (double) float_ptr[i];
}


void util_double_to_float(float *float_ptr , const double *double_ptr , int size) {
  int i;
  for (i=0; i < size; i++)
    float_ptr[i] = (float) double_ptr[i];
}


/*****************************************************************/

void util_fwrite_string(const char * s, FILE *stream) {
  int len = strlen(s);
  fwrite(&len , sizeof len , 1       , stream);
  fwrite(s    , 1          , len + 1 , stream);
}



char * util_fread_alloc_string(FILE *stream) {
  int len;
  char *s;
  fread(&len , sizeof len , 1 , stream);
  s = malloc(len + 1);
  fread(s , 1 , len + 1 , stream);
  return s;
}


void util_fskip_string(FILE *stream) {
  int len;
  fread(&len , sizeof len , 1 , stream);
  fseek(stream , len + 1 , SEEK_CUR);
}


/*****************************************************************/


int util_int_min(int a , int b) {
  return (a < b) ? a : b;
}

double util_double_min(double a , double b) {
  return (a < b) ? a : b;
}

float util_float_min(float a , float b) {
  return (a < b) ? a : b;
}

int util_int_max(int a , int b) {
  return (a > b) ? a : b;
}

double util_double_max(double a , double b) {
  return (a > b) ? a : b;
}

float util_float_max(float a , float b) {;
  return (a > b) ? a : b;
}


/*****************************************************************/



void util_endian_flip_vector(void *data, int element_size , int elements) {
  int i;
  switch (element_size) {
  case(1):
    break;
  case(2): 
    {
      uint16_t *tmp_int = (uint16_t *) data;
      for (i = 0; i <elements; i++)
	tmp_int[i] = FLIP16(tmp_int[i]);
      break;
    }
  case(4):
    {
      uint32_t *tmp_int = (uint32_t *) data;
      for (i = 0; i <elements; i++)
	tmp_int[i] = FLIP32(tmp_int[i]);
      break;
    }
  case(8):
    {
      uint64_t *tmp_int = (uint64_t *) data;
      for (i = 0; i <elements; i++)
	tmp_int[i] = FLIP64(tmp_int[i]);
      break;
    }
  default:
    fprintf(stderr,"%s can only 1/2/4/8 byte variables - aborting \n",__func__);
    abort();
  }
}

/*****************************************************************/

bool util_proc_alive(pid_t pid) {
  char proc_path[16];
  sprintf(proc_path , "/proc/%d" , pid);
  return util_path_exists(proc_path);
}

/*****************************************************************/


#define ABORT_READ  1
#define ABORT_WRITE 2

static FILE * util_fopen__(const char *filename , bool readonly, int abort_mode) {
  FILE *stream;

  if (readonly) {
    stream = fopen(filename , "r");
    if (stream == NULL) {
      fprintf(stderr,"%s: failed to open:%s for reading.\n",__func__ , filename);
      if (abort_mode & ABORT_READ) abort();
    }
  } else {
    stream = fopen(filename , "w");
    if (stream == NULL) {
      fprintf(stderr,"%s: failed to open:%s for writing.\n",__func__ , filename);
      if (abort_mode & ABORT_WRITE) abort();
    }
  }
  return stream;
}


FILE * util_fopen(const char * filename , bool readonly) {
  return util_fopen__(filename , readonly , ABORT_READ + ABORT_WRITE);
}


#undef ABORT_READ
#undef ABORT_WRITE

/*****************************************************************/

  
