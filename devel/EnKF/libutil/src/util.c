#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
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

static bool EOL_CHAR(char c) {
  if (c == '\r' || c == '\n')
    return true;
  else
    return false;
}


bool util_intptr_2bool(const int *iptr) {
  if ( (*iptr) == 0)
    return false;
  else
    return true;
}



void util_pad_f90string(char *c_string , int f90_length) {
  int i;
  for (i=strlen(c_string); i < f90_length; i++)
    c_string[i] = ' ';
}



char * util_alloc_cstring(const char *fort_string , const int *strlen) {
  const char null_char = '\0';
  char *new_string = malloc(*strlen + 1);
  strncpy(new_string , fort_string , *strlen);
  new_string[*strlen] = null_char;
  return new_string;
}


char * util_alloc_string_sum(const char ** string_list , int N) {
  int i , len;
  char * buffer;
  len = 0;
  for (i=0; i < N; i++) {
    if (string_list[i] != NULL)
      len += strlen(string_list[i]);
  }
  buffer = malloc(len + 1);
  buffer[0] = '\0';
  for (i=0; i < N; i++) {
    if (string_list[i] != NULL)
      strcat(buffer , string_list[i]);
  }
  return buffer;
}


char * util_alloc_string_sum2(const char *s1 , const char *s2) {
  char * buffer;
  buffer = malloc(strlen(s1) + strlen(s2) + 1);
  buffer[0] = '\0';
  strcat(buffer , s1);
  strcat(buffer , s2);
  return buffer;
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


void util_rewind_line(FILE *stream) {
  bool at_eol = false;
  int c;

  do {
    if (ftell(stream) == 0)
      at_eol = true;
    else {
      fseek(stream , -1 , SEEK_CUR);
      c = fgetc(stream);
      at_eol = EOL_CHAR(c);
      if (!at_eol)
	fseek(stream , -1 , SEEK_CUR);
    }
  } while (!at_eol);
}




static char * util_fscanf_alloc_line__(FILE *stream , bool *at_eof , char * line) {
  int init_pos = ftell(stream);
  char * new_line;
  int len;
  char end_char;
  bool cont;
  bool dos_newline;
  
  len  = 0;
  cont = true;
  {
    char c;
    do {
      c = fgetc(stream);
      if (c == EOF) 
	cont = false;
      else {
	if (EOL_CHAR(c))
	  cont = false;
	else
	  len++;
      }
    } while (cont);
    if (c == '\r')
      dos_newline = true;
    else
      dos_newline = false;
    end_char = c;
  }
  
  fseek(stream , init_pos , SEEK_SET);
  new_line = realloc(line , len + 1);
  util_fread(new_line , sizeof * new_line , len , stream , __func__);
  new_line[len] = '\0';
    
  /*
    Skipping the end of line marker(s).
  */
  fgetc(stream);
  if (dos_newline)
    fgetc(stream); 
 
  if (at_eof != NULL) {
    if (end_char == EOF)
      *at_eof = true;
    else
      *at_eof = false;
  }
  return new_line;
}


char * util_fscanf_alloc_line(FILE *stream , bool *at_eof) {
  return util_fscanf_alloc_line__(stream , at_eof , NULL);
}


char * util_fscanf_realloc_line(FILE *stream , bool *at_eof , char *line) {
  return util_fscanf_alloc_line__(stream , at_eof , line);
}

static time_t __util_fscanf_date(FILE *stream , bool abort_on_error , bool *error) {
  int day,month,year;
  char sep1,sep2;
  
  if (fscanf(stream,"%d%c%d%c%d" , &day , &sep1 , &month , &sep2 , &year) == 5) 
    return util_make_time1(day , month , year );  
  else {
    if (abort_on_error) {
      fprintf(stderr,"%s: failed to parse a valid date  - aborting \n",__func__);
      abort();
    }
    if (error != NULL)
      *error = true;
    return -1;
  }
}

/* 
   The date format is HARD assumed to be

   dd/mm/yyyy

   Where mm is [1..12]
   yyyy ~2000

*/



time_t util_fscanf_date(FILE * stream) {
  return __util_fscanf_date(stream , true , NULL);
}


bool util_fscanf_try_date(FILE * stream) {
  long int start_pos = ftell(stream);
  bool error;
  __util_fscanf_date(stream , true , &error);
  fseek(stream , start_pos , SEEK_SET);
  return error;
}


void util_fskip_lines(FILE * stream , int lines) {
  bool cont = true;
  int line_nr = 0;
  do {
    bool at_eof = false;
    char c;
    do {
      c = fgetc(stream);
      if (c == EOF)
	at_eof = true;
    } while (c != '\r' && c != '\n' && !at_eof);
    line_nr++;
    if (line_nr == lines || at_eof) cont = false;
  } while (cont);
}
	    

/*
  The last line(s) without content are not counted, i.e.

  File:
  ----------
  |Line1
  |Line2
  |
  |Line4
  |empty1
  |empty2
  |empty3

  will return a value of four.
*/
  
int util_forward_line(FILE * stream , bool * at_eof) {
  bool at_eol = false;
  int col = 0;
  do {
    char c = fgetc(stream);
    if (c == EOF) {
      *at_eof = true;
      at_eol  = true;
    } else {
      if (EOL_CHAR(c)) {
	at_eol = true;
	c = fgetc(stream);
	if (c == EOF) 
	  *at_eof = true;
	else {
	  if (!EOL_CHAR(c)) 
	    fseek(stream , -1 , SEEK_CUR);
	}
      } else
	col++;
    }
  } while (!at_eol);
  return col;
}



bool util_char_in(char c , int set_size , const char * set) {
  int i;
  bool in = false;
  for (i=0; i < set_size; i++)
    if (set[i] == c)
      in = true;
  
  return in;
}


void util_fskip_token(FILE * stream) {
  char * tmp = util_fscanf_alloc_token(stream);
  if (tmp != NULL)
    free(tmp);
}


char * util_fscanf_alloc_token(FILE * stream) {
  const char * space_set = " \t";
  bool cont;
  char * token = NULL;
  char c;

  
  cont = true;

  /* Skipping initial whitespace */
  do {
    int pos = ftell(stream);
    c = fgetc(stream);
    if (EOL_CHAR(c)) {
      /* Going back to position at newline */
      fseek(stream , pos , SEEK_SET);
      cont = false;
    } else if (c == EOF) 
      cont = false;
    else if (!util_char_in(c , 2 , space_set)) {
      fseek(stream , pos , SEEK_SET);
      cont = false;
    }
  } while (cont);
  if (EOL_CHAR(c)) return NULL;
  if (c == EOF)    return NULL;

  

  /* At this point we are guranteed to return something != NULL */
  cont = true;
  {
    int length = 0;
    long int token_start = ftell(stream);

    do {
      c = fgetc(stream);
      if (c == EOF)
	cont = false;
      else if (EOL_CHAR(c))
	cont = false;
      else if (util_char_in(c , 2 , space_set))
	cont = false;
      else
	length++;
    } while (cont);
    if (EOL_CHAR(c)) fseek(stream , -1 , SEEK_CUR);
  
    token = malloc(length + 1);
    fseek(stream , token_start , SEEK_SET);
    { 
      int i;
      for (i = 0; i < length; i++)
	token[i] = fgetc(stream);
      token[length] = '\0';
    }
  }
  return token;
}


bool util_fscanf_int(FILE * stream , int * value) {
  long int start_pos = ftell(stream);
  char * token       = util_fscanf_alloc_token(stream);
  char * error_ptr;
  bool   value_OK = false;
  
  if (token != NULL) {
    int tmp_value = strtol(token , &error_ptr , 10);
    if (error_ptr[0] == '\0') {
      value_OK = true; 
      *value = tmp_value;
    } else
      fseek(stream , start_pos , SEEK_SET);
    free(token);
  }
  return value_OK;
}   



int util_count_file_lines(FILE * stream) {
  long int init_pos = ftell(stream);
  int lines = 0;
  bool at_eof = false;
  do {
    int col = util_forward_line(stream , &at_eof);
    if (col > 0) lines++;
  } while (!at_eof);
  fseek(stream , init_pos , SEEK_SET);
  return lines;
}



/* int util_count_file_lines(FILE * stream) { */
/*   int lines       = 0; */
/*   int empty_lines = 0; */
/*   int col         = 0; */
/*   char    c; */
  
/*   do { */
/*     c = fgetc(stream); */
/*     if (EOL_CHAR(c)) { */
/*       printf("lines: %d   empty_lines:%d EOL \n",lines,empty_lines); */
/*       if (col == 0) */
/* 	empty_lines++; */
/*       else { */
/* 	lines       += empty_lines + 1; */
/* 	empty_lines  = 0; */
/*       } */
/*       col = 0; */
/*       c = fgetc(stream); */
/*       if ( !feof(stream) ) { */
/* 	if (! EOL_CHAR(c) )  */
/* 	  fseek(stream , -1 , SEEK_CUR); */
/*       } */
/*     } else if (c == EOF) */
/*       lines++; */
/*     else  */
/*       col++; */
/*   } while (! feof(stream) ); */
/*   if (col == 0)  */
/*     /\* */
/*       Not counting empty last line. */
/*     *\/ */
/*     lines--; */
/*   printf("Returning: lines:%d \n",lines); */
/*   return lines; */
/* } */


int util_count_content_file_lines(FILE * stream) {
  int lines       = 0;
  int empty_lines = 0;
  int col         = 0;
  char    c;
  
  do {
    c = fgetc(stream);
    if (EOL_CHAR(c)) {
      if (col == 0)
	empty_lines++;
      else {
	lines       += empty_lines + 1;
	empty_lines  = 0;
      }
      col = 0;
      c = fgetc(stream);
      if (! feof(stream) ) {
	if (!EOL_CHAR(c))
	  fseek(stream , -1 , SEEK_CUR);
      }
    } else if (c == EOF)
      lines++;
    else {
      if (c != ' ')
	col++;
    }
  } while (! feof(stream) );
  if (col == 0) 
    /*
      Not counting empty last line.
    */
    lines--;
  return lines;
}


/******************************************************************/

void util_copy_stream(FILE *src_stream , FILE *target_stream , int buffer_size , void * buffer) {

  while ( ! feof(src_stream)) {
    int bytes_read;
    int bytes_written;
    bytes_read = fread (buffer , 1 , buffer_size , src_stream);
    
    if (bytes_read < buffer_size && !feof(src_stream)) {
      fprintf(stderr,"%s: error when reading from src_stream - aborting \n",__func__);
      abort();
    }

    bytes_written = fwrite(buffer , 1 , bytes_read , target_stream);

    if (bytes_written < bytes_read) {
      fprintf(stderr,"%s: not all bytes written to target stream - aboring \n",__func__);
      abort();
    }
  }

}


void util_copy_file(const char * src_file , const char * target_file) {
  if (strcmp(src_file , target_file) == 0) 
    fprintf(stderr,"%s Warning: trying to copy %s onto itself - noting done\n",__func__ , src_file);
  else {
    const int buffer_size  = 65536;
    char * buffer          = malloc(buffer_size);
    FILE * src_stream      = util_fopen(src_file    , "r");
    FILE * target_stream   = util_fopen(target_file , "w");
    
    util_copy_stream(src_stream , target_stream , buffer_size , buffer);
    fclose(src_stream);
    fclose(target_stream);
    free(buffer);
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


bool util_is_directory(const char * path) {
  struct stat stat_buffer;
  stat(path , &stat_buffer);
  return S_ISDIR(stat_buffer.st_mode);
}


bool util_is_link(const char * path) {
  struct stat stat_buffer;
  lstat(path , &stat_buffer);
  return S_ISLNK(stat_buffer.st_mode);
}





int util_get_path_length(const char * file) {
  if (util_is_directory(file)) 
    return strlen(file);
  else {
    char * last_slash = strrchr(file , '/');
    if (last_slash == NULL)
      return 0;
    else
      return last_slash - file;
  }
}



int util_get_base_length(const char * file) {
  int path_length   = util_get_path_length(file);
  char * base_start , *last_point;

  if (path_length == strlen(file))
    return 0;
  else if (path_length == 0)
    base_start = (char *) file;
  else 
    base_start = (char *) &file[path_length + 1];
  
  last_point  = strrchr(base_start , '.');
  if (last_point == NULL)
    return strlen(base_start);
  else
    return last_point - base_start;
}






void util_alloc_file_components(const char * file, char **_path , char **_basename , char **_extension) {
  char *path      = NULL;
  char *basename  = NULL;
  char *extension = NULL;
  
  int path_length = util_get_path_length(file);
  int base_length = util_get_base_length(file);
  int ext_length ;
  int slash_length = 1;


  if (path_length > 0) 
    path = util_alloc_substring_copy(file , path_length);
  else
    slash_length = 0;


  if (base_length > 0)
    basename = util_alloc_substring_copy(&file[path_length + slash_length] , base_length);

  
  ext_length = strlen(file) - (path_length + base_length + 1);
  if (ext_length > 0)
    extension = util_alloc_substring_copy(&file[path_length + slash_length + base_length + 1] , ext_length);

  
  if (_extension != NULL) *_extension = extension;
  if (_basename  != NULL) *_basename  = basename;
  if (_path      != NULL) *_path      = path;
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


bool util_same_file(const char * file1 , const char * file2) {
  struct stat buffer1 , buffer2;
  
  stat(file1, &buffer1);
  stat(file2, &buffer2);

  if (buffer1.st_ino == buffer2.st_ino) 
    return true;
  else
    return false;
}


void util_make_slink(const char *target , const char * link) {
  if (symlink(target , link) != 0) {
    fprintf(stderr,"%s: linking %s -> %s failed - aborting \n",__func__ , link , target);
    fprintf(stderr,"%s\n",strerror(errno));
    abort();
  }
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
    if (path[0] == '/')
      offset = 1;
    else
      offset = 0;
    length      = strcspn(&path[offset] , "/");
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
      sprintf(file , "%s/%s-%d-%ld" , path , prefix , pid , rand_int);
    else
      sprintf(file , "%s/%s-%ld" , path , prefix , rand_int);
  } while (util_file_exists(file));
  return file;
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



static void __util_set_timevalues(time_t t , int * sec , int * min , int * hour , int * mday , int * month , int * year) {
  struct tm ts;

  localtime_r(&t , &ts);
  if (sec   != NULL) *sec   = ts.tm_sec;
  if (min   != NULL) *min   = ts.tm_min;
  if (hour  != NULL) *hour  = ts.tm_hour;
  if (mday  != NULL) *mday  = ts.tm_mday;
  if (month != NULL) *month = ts.tm_mon  + 1;
  if (year  != NULL) *year  = ts.tm_year + 1900;
}

void util_set_datetime_values(time_t t , int * sec , int * min , int * hour , int * mday , int * month , int * year) {
  __util_set_timevalues(t , sec , min , hour , mday , month , year);
}


void util_set_date_values(time_t t , int * mday , int * month , int * year) {
  __util_set_timevalues(t , NULL , NULL , NULL , mday , month , year);
}


/*
  Observe that this routine does the following transform before calling mktime:

    1. month -> month - 1;
    2. year  -> year  - 1900;

  Then it is on the format which mktime expects.

*/

time_t util_make_time2(int sec, int min, int hour , int mday , int month , int year) {
  struct tm ts;
  ts.tm_sec    = sec;
  ts.tm_min    = min;
  ts.tm_hour   = hour;
  ts.tm_mday   = mday;
  ts.tm_mon    = month - 1;
  ts.tm_year   = year  - 1900;
  {
    time_t t = mktime( &ts );
    if (t == -1) {
      fprintf(stderr,"%s: failed to make a time_t instance of %02d/%02d%4d  %02d:%02d:%02d - aborting \n",__func__ , mday,month,year,hour,min,sec);
      abort();
    }
    return t;
  }
}



time_t util_make_time1(int mday , int month , int year) {
  return util_make_time2(0 , 0 , 0 , mday , month , year);
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


char ** util_alloc_stringlist_copy(const char **src, int len) {
  if (src != NULL) {
    int i;
    char ** copy = calloc(len , sizeof * copy);
    for (i=0; i < len; i++)
      copy[i] = util_alloc_string_copy(src[i]);
    return copy;
  } else 
    return NULL;
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
    char *cwd       = NULL;
    struct dirent *dentry;
    cwd  = getcwd(cwd , 0);
    if (chdir(path) != 0) {
      fprintf(stderr,"%s: failed to change to %s - aborting \n", __func__ , path);
      abort();
    } else {
      DIR *dirH;
      dirH = opendir( "./" );  /* Have already changed into this directory with chdir() */
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
      closedir(dirH);
      chdir(cwd);
      free(cwd);

      if (rmdir(path) != 0) 
	fprintf(stderr,"%s: Warning: failed to remove director:%s \n",__func__ , path);
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
    double_ptr[i] = float_ptr[i];
}


void util_double_to_float(float *float_ptr , const double *double_ptr , int size) {
  int i;
  for (i=0; i < size; i++)
    float_ptr[i] = double_ptr[i];
}


/* 
   Stride is a fucking mess here ....
*/

size_t util_pack_vector(const void * _src, const bool * active_src , size_t src_size , int src_stride , void * _target , int target_stride , size_t target_size ,  int type_size , bool * complete) {
  const char * src    = (const char *) _src;
  char       * target = (char *)       _target;
  
  size_t src_index;
  size_t target_index = 0;

  printf("%s: stride confusion in call \n",__func__); abort();

  for (src_index = 0; src_index < src_size; src_index++) {
    if (active_src[src_index]) {
      size_t src_adress    = src_index    * type_size * src_stride;
      size_t target_adress = target_index * type_size * target_stride;
      memcpy(&target[target_adress] , &src[src_adress] , type_size);
      target_index++;
      if (target_index == target_size) {
	if (src_index < (src_size - 1)) *complete = false;
	break;
      }
    }
  }
  return target_index;
}



size_t util_unpack_vector(const void * _src, int src_size , int src_stride , void * _target , const bool * active_target , size_t target_size , int target_stride , int type_size) {
  const char * src    = (const char *) _src;
  char       * target = (char *)       _target;
  
  size_t src_index = 0;
  size_t target_index;
  size_t new_target_offset = 0;

  for (target_index = 0; target_index < target_size; target_index++) {
    if (active_target[target_index]) {
      size_t src_adress    = src_index    * type_size * src_stride;
      size_t target_adress = target_index * type_size * target_stride;
      memcpy(&target[target_adress] , &src[src_adress] , type_size);
      src_index++;
      if (src_index == src_size) {
	new_target_offset = target_index + 1;
	break;
      }
    }
  }
  return new_target_offset;
}


size_t util_copy_strided_vector(const void * _src, size_t src_size , int src_stride , void * _target , int target_stride , size_t target_size ,  int type_size , bool * complete) {
  const char * src    = (const char *) _src;
  char       * target = (char *)       _target;
  
  size_t src_index;
  size_t target_index = 0;

  for (src_index = 0; src_index < src_size; src_index++) {
    size_t src_adress    = src_index    * type_size * src_stride;
    size_t target_adress = target_index * type_size * target_stride;
    memcpy(&target[target_adress] , &src[src_adress] , type_size);
    target_index++;
    if (target_index == target_size) {
      if (src_index < (src_size - 1)) *complete = false;
      break;
    }
  }
  return target_index;
}





/*****************************************************************/

void util_fwrite_string(const char * s, FILE *stream) {
  int len = 0;
  if (s != NULL) {
    len = strlen(s);
    util_fwrite(&len , sizeof len , 1       , stream , __func__);
    util_fwrite(s    , 1          , len + 1 , stream , __func__);
  } else
    util_fwrite(&len , sizeof len , 1       , stream , __func__);
}



char * util_fread_alloc_string(FILE *stream) {
  int len;
  char *s = NULL;
  util_fread(&len , sizeof len , 1 , stream , __func__);
  if (len > 0) {
    s = malloc(len + 1);
    util_fread(s , 1 , len + 1 , stream , __func__);
  } 
  return s;
}


void util_fskip_string(FILE *stream) {
  int len;
  util_fread(&len , sizeof len , 1 , stream , __func__);
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


int util_proc_mem_free(void) {
  FILE *stream = util_fopen("/proc/meminfo" , "r");
  int mem;
  util_fskip_lines(stream , 1);
  util_fskip_token(stream);
  util_fscanf_int(stream , &mem);
  fclose(stream);
  return mem;
}


/*****************************************************************/


#define ABORT_READ  1
#define ABORT_WRITE 2

static FILE * util_fopen__(const char *filename , const char * mode, int abort_mode) {
  FILE *stream;

  if (strcmp(mode , "r") == 0) {
    stream = fopen(filename , "r");
    if (stream == NULL) {
      fprintf(stderr,"%s: failed to open:%s for reading: %s \n",__func__ , filename , strerror(errno));
      if (abort_mode & ABORT_READ) abort();
    }
  } else if (strcmp(mode ,"w") == 0) {
    stream = fopen(filename , "w");
    if (stream == NULL) {
      fprintf(stderr,"%s: failed to open:%s for writing: %s \n",__func__ , filename, strerror(errno));
      if (abort_mode & ABORT_WRITE) abort();
    }
  } else {
    fprintf(stderr,"%s: open mode: '%s' not implemented - aborting \n",__func__ , mode);
    abort();
  }
  return stream;
}


FILE * util_fopen(const char * filename , const char * mode) {
  return util_fopen__(filename , mode , ABORT_READ + ABORT_WRITE);
}


void util_fwrite(const void *ptr , size_t element_size , size_t items, FILE * stream , const char * caller) {
  int items_written = fwrite(ptr , element_size , items , stream);
  if (items_written != items) {
    fprintf(stderr,"%s/%s: only wrote %d/%d items to disk - aborting.\n",caller , __func__ , items_written , items);
    fprintf(stderr,"%s\n",strerror(errno));
    abort();
  }
}


void util_fread(void *ptr , size_t element_size , size_t items, FILE * stream , const char * caller) {
  int items_read = fread(ptr , element_size , items , stream);
  if (items_read != items) {
    fprintf(stderr,"%s/%s: only read %d/%d items from disk - aborting.\n",caller , __func__ , items_read , items);
    fprintf(stderr,"%s\n",strerror(errno));
    abort();
  }
}

#undef ABORT_READ
#undef ABORT_WRITE


/*****************************************************************/


void * util_malloc(size_t size , const char * caller) {
  void *data = malloc(size);
  if (data == NULL) {
    fprintf(stderr,"%s: failed to allocate %d bytes - aborting \n",caller , size);
    abort();
  }
  return data;
}


static void util_display_prompt(const char * prompt , int prompt_len2) {
  int i;
  printf("%s" , prompt);
  for (i=0; i < util_int_max(strlen(prompt) , prompt_len2) - strlen(prompt); i++)
    fputc(' ' , stdout);
  printf(": ");
}



void util_read_string(const char * prompt , int prompt_len , char * s) {
  util_display_prompt(prompt , prompt_len);
  fscanf(stdin , "%s" , s);
}


void util_read_path(const char * prompt , int prompt_len , bool must_exist , char * path) {
  bool ok = false;
  while (!ok) {
    util_read_string(prompt , prompt_len , path);
    if (must_exist)
      ok = util_path_exists(path);
    else
      ok = true;
    if (!ok) 
      fprintf(stderr,"Path: %s does not exist - try again.\n",path);
  }
}


/*void util_read_file(const char * _prompt , const char * path , bool must_exist , char * file) {
  char * prompt = util_alloc_string_sum2(_prompt , path);
  char _file[256];
  bool ok = false;
  while (!ok) {
    util_read_string(prompt , strlen(prompt) + 5 , _file);
    if (must_exist)
      ok = util_path_exists(path);
    else
      ok = true;
    if (!ok) 
      fprintf(stderr,"Path: %s does not exist - try again.\n",path);
  }
}
*/
  
