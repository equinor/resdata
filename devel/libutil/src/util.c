/**

  This file contains a large number of utility functions for memory
  handling, string handling and file handling. Observe that all these
  functions are just that - functions - there is no associated state
  with any of these functions.

  The file util_path.c is included in this, and contains path
  manipulation functions which explicitly use the PATH_SEP
*/


#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <util.h>
#include <zlib.h>
#include <math.h>
#include <hash.h>
#include <set.h>
#include <void_arg.h>
#include <stdarg.h>
#include <execinfo.h>


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


char * util_alloc_cstring(const char *fort_string , const int *strlen) {
  const char null_char = '\0';
  char *new_string = util_malloc(*strlen + 1 , __func__);
  strncpy(new_string , fort_string , *strlen);
  new_string[*strlen] = null_char;
  return new_string;
}


void util_pad_f90string(char *string , int c_strlen , int f90_length) {
  int i;
  for (i=c_strlen; i < f90_length; i++) 
    string[i] = ' ';
}



bool util_intptr_2bool(const int *iptr) {
  if ( (*iptr) == 0)
    return false;
  else
    return true;
}



int util_C2f90_bool(bool c_bool) {
  /* This is true for the ifort compiler ... */
  if (c_bool)
    return -1;
  else
    return 0;
}
  



void util_memcpy_string_C2f90(const char * c_input_string , char * fortran_output_string , int fortran_length) {
  strncpy(fortran_output_string , c_input_string , strlen(c_input_string));
  util_pad_f90string(fortran_output_string , strlen(c_input_string) , fortran_length);
}


char * util_alloc_string_sum2(const char *s1 , const char *s2) {
  char * buffer;
  buffer = util_malloc(strlen(s1) + strlen(s2) + 1 , __func__);
  buffer[0] = '\0';
  strcat(buffer , s1);
  strcat(buffer , s2);
  return buffer;
}





char * util_alloc_substring_copy(const char *src , int N) {
  char *copy;
  if (N < strlen(src)) {
    copy = util_malloc(N + 1 , __func__);
    strncpy(copy , src , N);
    copy[N] = '\0';
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

void util_strupr(char *s) {
  int i;
  for (i=0; i < strlen(s); i++)
    s[i] = toupper(s[i]);
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
  
  {
    char * strip_line = util_alloc_strip_copy(new_line);
    free(new_line);
    
    return strip_line;
  }
}


char * util_fscanf_alloc_line(FILE *stream , bool *at_eof) {
  return util_fscanf_alloc_line__(stream , at_eof , NULL);
}


char * util_fscanf_realloc_line(FILE *stream , bool *at_eof , char *line) {
  return util_fscanf_alloc_line__(stream , at_eof , line);
}


char * util_alloc_cwd(void) {
  char * result_ptr;
  char * cwd;
  int buffer_size = 128;
  do {
    cwd = util_malloc(buffer_size , __func__);
    result_ptr = getcwd(cwd , buffer_size - 1);
    if (result_ptr == NULL) {
      if (errno == ERANGE) {
	buffer_size *= 2;
	free(cwd);
      }
    }
  } while ( result_ptr == NULL );
  cwd = util_realloc(cwd , strlen(cwd) + 1 , __func__);
  return cwd;
}



bool util_sscanf_date(const char * date_token , time_t * t) {
  int day   , month , year;
  char sep1 , sep2;
  
  if (sscanf(date_token , "%d%c%d%c%d" , &day , &sep1 , &month , &sep2 , &year) == 5) {
    *t = util_make_time1(day , month , year );  
    return true;
  } else {
    *t = -1;
    return false;
  }
}


bool util_fscanf_date(FILE *stream , time_t *t)  {
  int init_pos = ftell(stream);
  char * date_token = util_fscanf_alloc_token(stream);
  bool return_value = util_sscanf_date(date_token , t);
  if (!return_value) fseek(stream , init_pos , SEEK_SET);
  free(date_token);
  return return_value;
}

/* 
   The date format is HARD assumed to be

   dd/mm/yyyy

   Where mm is [1..12]
   yyyy ~2000

*/







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


static void util_fskip_chars__(FILE * stream , const char * skip_set , bool complimentary_set , bool *at_eof) {
  int set_len = strlen(skip_set);
  bool in_set;
  do {
    int i;
    int c = fgetc(stream);
    in_set = false;
    if (c == EOF) 
      *at_eof = true;
    else {
      for (i=0; i < set_len; i++) {
	if (skip_set[i] == c)
	  in_set = true;
      }
    }
  } while (in_set != complimentary_set);
  fseek(stream , -1 , SEEK_CUR);
}


void util_fskip_chars(FILE * stream , const char * skip_set , bool *at_eof) {
  util_fskip_chars__(stream , skip_set , false , at_eof);
}


void util_fskip_cchars(FILE * stream , const char * skip_set , bool *at_eof) {
  util_fskip_chars__(stream , skip_set , true , at_eof);
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
  
    token = util_malloc(length + 1 , __func__);
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


bool util_sscanf_int(const char * buffer , int * value) {
  bool value_OK = false;
  char * error_ptr;

  int tmp_value = strtol(buffer , &error_ptr , 10);
  if (error_ptr[0] == '\0') {
    value_OK = true; 
    *value = tmp_value;
  } 
  return value_OK;
}


bool util_fscanf_int(FILE * stream , int * value) {
  long int start_pos = ftell(stream);
  char * token       = util_fscanf_alloc_token(stream);
  
  bool   value_OK = false;
  if (token != NULL) {
    value_OK = util_sscanf_int(token , value);
    if (!value_OK)
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


char * util_fread_alloc_file_content(const char * filename , const char * comment, int * buffer_size) {
  int file_size = util_file_size(filename);
  char * buffer = util_malloc(file_size + 1 , __func__);
  FILE * stream = util_fopen(filename , "r");
  int byte_read = fread(buffer , 1 , file_size , stream);
  if (byte_read != file_size) 
    util_abort("%s: something failed when reading: %s - aborting \n",__func__ , filename);
  
  fclose(stream);
  
  if (comment != NULL) {
    char * src_buffer    = buffer;
    char * target_buffer = util_malloc(file_size + 1 , __func__);
    bool comment_on      = false;
    int src_pos          = 0;
    int target_pos       = 0;
    int comment_length;

    do {
      if (!comment_on) {
	/* Turning on comment */
	if (strncmp(&src_buffer[src_pos] , comment , strlen(comment)) == 0) {
	  comment_on     = true;
	  comment_length = 0;
	} else {
	  /*Plain character transfer */
	  target_buffer[target_pos] = src_buffer[src_pos];
	  target_pos++;
	}
      } else {
	/* We are in a comment - should maybe turn it off?? Turning off on \n */
	if (src_buffer[src_pos] == '\n') {
	  comment_on = false;
	  if (src_buffer[src_pos - 1] == '\r') {
	    target_buffer[target_pos] = '\r';
	    target_pos++;
	  }
	  target_buffer[target_pos] = '\n';
	  target_pos++;
	} else 
	  comment_length++;
      }
      src_pos++;
    } while (src_pos <= file_size); 
    target_buffer[target_pos] = '\0';
    free(src_buffer);
    
    target_buffer = realloc(target_buffer , strlen(target_buffer) + 1);
    *buffer_size  = strlen(target_buffer);
    return target_buffer;
  } else {
    *buffer_size = file_size;
    buffer[file_size] = '\0';
    return buffer;
  }
}




void util_copy_stream(FILE *src_stream , FILE *target_stream , int buffer_size , void * buffer) {

  while ( ! feof(src_stream)) {
    int bytes_read;
    int bytes_written;
    bytes_read = fread (buffer , 1 , buffer_size , src_stream);
    
    if (bytes_read < buffer_size && !feof(src_stream)) 
      util_abort("%s: error when reading from src_stream - aborting \n",__func__);

    bytes_written = fwrite(buffer , 1 , bytes_read , target_stream);

    if (bytes_written < bytes_read) 
      util_abort("%s: not all bytes written to target stream - aboring \n",__func__);
  }

}


void util_copy_file(const char * src_file , const char * target_file) {
  if (strcmp(src_file , target_file) == 0) 
    fprintf(stderr,"%s Warning: trying to copy %s onto itself - noting done\n",__func__ , src_file);
  else {
    void * buffer   = NULL;
    int buffer_size = util_file_size(src_file);
    do {
      buffer = malloc(buffer_size);
      if (buffer == NULL) buffer_size /= 2;
    } while (buffer == NULL);
    
    {
      FILE * src_stream      = util_fopen(src_file     , "r");
      FILE * target_stream   = util_fopen(target_file  , "w");
    
      util_copy_stream(src_stream , target_stream , buffer_size , buffer);
      fclose(src_stream);
      fclose(target_stream);
    }
    free(buffer);
  }
}


bool util_file_exists(const char *filename) {
  FILE *stream = fopen(filename , "r");
  bool ex;
  if (stream == NULL) {
    if (errno == ENOENT)
      ex = false;
    else 
      util_abort("file: %s exists but open failed - aborting \n",filename);
  } else {
    fclose(stream);
    ex = true;
  }
  return ex;
}


/*bool util_is_file(const char * path) {
  struct stat stat_buffer;
  stat(path , &stat_buffer);
  return S_ISFILE(stat_buffer.st_mode);
}
*/



/**
  This function return true if path corresponds to an existing
  directory, if stat() fail errno is checked for the value ENOENT, in
  which case a false is returned.
*/
bool util_is_directory(const char * path) {
  struct stat stat_buffer;

  if (stat(path , &stat_buffer) == 0)
    return S_ISDIR(stat_buffer.st_mode);
  else if (errno == ENOENT)
    /*Path does not exist at all. */
    return false;
  else {
    util_abort("%s: stat(%s) failed: %s \n",__func__ , path , strerror(errno));
    return false; /* Dummy to shut the compiler warning */
  }
}




/**
  This function returns true if path is a symbolic link.
*/
bool util_is_link(const char * path) {
  struct stat stat_buffer;
  if (lstat(path , &stat_buffer) == 0)
    return S_ISLNK(stat_buffer.st_mode);
  else if (errno == ENOENT)
    /*Path does not exist at all. */
    return false;
  else {
    util_abort("%s: stat(%s) failed: %s \n",__func__ , path , strerror(errno));
    return false;
  }
}


bool util_is_executable(const char * path) {
  if (util_file_exists(path)) {
    struct stat stat_buffer;
    stat(path , &stat_buffer);
    return (stat_buffer.st_mode & S_IXUSR);
  } else {
    util_abort("%s: file:%s does not exist - aborting \n",__func__ , path);
    return false; /* Dummy to shut up compiler */
  }
}


int util_get_base_length(const char * file) {
  int path_length   = util_get_path_length(file);
  const char * base_start;
  const char * last_point;

  if (path_length == strlen(file))
    return 0;
  else if (path_length == 0)
    base_start = file;
  else 
    base_start = &file[path_length + 1];
  
  last_point  = strrchr(base_start , '.');
  if (last_point == NULL)
    return strlen(base_start);
  else
    return last_point - base_start;

}




/**
   This function splits a filename into three parts:

    1. A leading path.
    2. A base name.
    3. An extension.

   In the calling scope you should pass in references to pointers to
   char for the fields, you are interested in:

   Example:
   --------

   char * path;
   char * base;
   char * ext;

   util_alloc_file_components("/path/to/some/file.txt" , &path , &base , &ext);
   util_alloc_file_components("/path/to/some/file.txt" , &path , &base , NULL); 

   In the second example we were not interested in the extension, and
   just passed in NULL. Before use in the calling scope it is
   essential to check the values of base, path and ext:

   util_alloc_file_components("/path/to/some/file" , &path , &base , &ext);
   if (ext != NULL)
      printf("File: has extension: %s \n",ext);
   else
      printf("File does *not* have an extension \n");

   The memory allocated in util_alloc_file_components must be freed by
   the calling unit.
*/

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

  if (_extension != NULL) 
    *_extension = extension;
  else 
    if (extension != NULL) free(extension);
  

  if (_basename != NULL) 
    *_basename = basename;
  else 
    if (basename != NULL) free(basename);
  

  if (_path != NULL) 
    *_path = path;
  else 
    if (path != NULL) free(path);
  

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
  if (fildes == -1) 
    util_abort("%s: failed to open:%s - %s \n",__func__ , file , strerror(errno));
  
  fstat(fildes, &buffer);
  close(fildes);
  
  return buffer.st_size;
}


static char * util_alloc_link_target(const char * link) {
  bool retry = true;
  int target_length;
  int buffer_size = 64;
  char * buffer = NULL;
  do {
    buffer = util_realloc(buffer , buffer_size , __func__);
    target_length = readlink(link , buffer , buffer_size);
    if (target_length == -1) 
      util_abort("%s: readlink(%s,...) failed with error:%s - aborting\n",__func__ , link , strerror(errno));

    if (target_length < buffer_size)
      retry = false;
    else
      buffer_size *= 2;

  } while (retry);
  buffer[target_length] = '\0';
  return buffer;
}



bool util_same_file(const char * file1 , const char * file2) {
  char * target1 = (char *) file1;
  char * target2 = (char *) file2;

  if (util_is_link(file1)) target1 = util_alloc_link_target(file1);
  if (util_is_link(file2)) target2 = util_alloc_link_target(file2);
  {
    struct stat buffer1 , buffer2;
    
    stat(target1, &buffer1);
    stat(target2, &buffer2);

    if (target1 != file1) free(target1);
    if (target2 != file2) free(target2);
    
    if (buffer1.st_ino == buffer2.st_ino) 
      return true;
    else
      return false;
  }
}




void util_make_slink(const char *target , const char * link) {
  if (util_file_exists(link)) {
    if (util_is_link(link)) {
      if (!util_same_file(target , link)) 
	util_abort("%s: %s already exists - not pointing to: %s - aborting \n",__func__ , link , target);
    } else 
      util_abort("%s: %s already exists - is not a link - aborting \n",__func__ , link);
  } else {
    if (symlink(target , link) != 0) 
      util_abort("%s: linking %s -> %s failed - aborting: %s \n",__func__ , link , target , strerror(errno));
  }
}



void util_unlink_existing(const char *filename) {
  if (util_file_exists(filename))
    unlink(filename);
}


bool util_fmt_bit8_stream(FILE * stream ) {
  const int    min_read      = 1024;
  const double bit8set_limit = 0.00001;
  const int    buffer_size   = 131072;
  long int start_pos         = ftell(stream);
  bool fmt_file;
  {
    double bit8set_fraction;
    int N_bit8set = 0;
    int elm_read,i;
    char *buffer = util_malloc(buffer_size , __func__);

    elm_read = fread(buffer , 1 , buffer_size , stream);
    if (elm_read < min_read) 
      util_abort("%s: file is too small to automatically determine formatted/unformatted status \n",__func__);
    
    for (i=0; i < elm_read; i++)
      N_bit8set += (buffer[i] & (1 << 7)) >> 7;
    
    
    free(buffer);
    bit8set_fraction = 1.0 * N_bit8set / elm_read;
    if (bit8set_fraction < bit8set_limit) 
      fmt_file =  true;
    else 
      fmt_file = false;
  }
  fseek(stream , start_pos , SEEK_SET);
  return fmt_file;
}  



bool util_fmt_bit8(const char *filename ) {
  FILE *stream;
  bool fmt_file = true;

  if (util_file_exists(filename)) {
    stream   = fopen(filename , "r");
    fmt_file = util_fmt_bit8_stream(stream);
    fclose(stream);
  } else 
    util_abort("%s: could not find file: %s - aborting \n",__func__ , filename);

  return fmt_file;
}



/*
  time_t        st_atime;    time of last access 
  time_t        st_mtime;    time of last modification 
  time_t        st_ctime;    time of last status change 
*/

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
    if (t == -1) 
      util_abort("%s: failed to make a time_t instance of %02d/%02d/%4d  %02d:%02d:%02d - aborting \n",__func__ , mday,month,year,hour,min,sec);
    
    return t;
  }
}



time_t util_make_time1(int mday , int month , int year) {
  return util_make_time2(0 , 0 , 0 , mday , month , year);
}



static int util_get_month_nr__(const char * _month_name) {
  int month_nr = -1;
  char * month_name = util_alloc_string_copy(_month_name);
  util_strupr(month_name);
  if (strncmp(month_name , "JAN" , 3)) 
    month_nr = 1;
  else if (strncmp(month_name , "FEB" , 3)) 
    month_nr = 2;
  else if (strncmp(month_name , "MAR" , 3)) 
    month_nr = 3;
  else if (strncmp(month_name , "APR" , 3)) 
    month_nr = 4;
  else if (strncmp(month_name , "MAI" , 3)) 
    month_nr = 5;
  else if (strncmp(month_name , "MAY" , 3)) 
    month_nr = 5;
  else if (strncmp(month_name , "JUN" , 3)) 
    month_nr = 6;
  else if (strncmp(month_name , "JUL" , 3)) 
    month_nr = 7;
  else if (strncmp(month_name , "AUG" , 3)) 
    month_nr = 8;
  else if (strncmp(month_name , "SEP" , 3)) 
    month_nr = 9;
  else if (strncmp(month_name , "OKT" , 3)) 
    month_nr = 10;
  else if (strncmp(month_name , "NOV" , 3)) 
    month_nr = 11;
  else if (strncmp(month_name , "DES" , 3)) 
    month_nr = 12;
  free(month_name);
  return month_nr;
}

int util_get_month_nr(const char * month_name) {
  int month_nr = util_get_month_nr__(month_name);
  if (month_nr < 0) 
    util_abort("%s: %s not a valid month name - aborting \n",__func__ , month_name);
  
  return month_nr;
}


int util_check_month(const char * month_name) {
  return util_get_month_nr__(month_name);
}


/*****************************************************************/

void util_set_strip_copy(char * copy , const char *src) {
  const char null_char  = '\0';
  const char space_char = ' ';
  int  src_index   = 0;
  int target_index = 0;
  while (src[src_index] == space_char)
    src_index++;

  while (src[src_index] != null_char && src[src_index] != space_char) {
    copy[target_index] = src[src_index];
    src_index++;
    target_index++;
  }
  copy[target_index] = null_char;
}


/**
   The function will allocate a new copy of src where leading and
   trailing whitespace has been stripped off. If the source string is
   all blanks a string of length one - only containing \0 is returned,
   i.e. not NULL.

   If src is NULL the function will return NULL. The incoming source
   string is not modified, see the function util_realloc_strip_copy()
   for a similar function implementing realloc() semantics.
*/


char * util_alloc_strip_copy(const char *src) {
  char * target;
  int strip_length = 0;
  int end_index   = strlen(src) - 1;
  while (end_index >= 0 && src[end_index] == ' ')
    end_index--;

  if (end_index >= 0) {

    int start_index = 0;
    while (src[start_index] == ' ')
      start_index++;
    strip_length = end_index - start_index + 1;
    target = util_malloc(strip_length + 1 , __func__);
    memcpy(target , &src[start_index] , strip_length);
  } else 
    /* A blank string */
    target = util_malloc(strip_length + 1 , __func__);

  target[strip_length] = '\0';
  return target;
}



char * util_realloc_strip_copy(char *src) {
  if (src == NULL)
    return NULL;
  else {
    char * strip_copy = util_alloc_strip_copy(src);
    free(src);
    return strip_copy;
  }
}


char ** util_alloc_stringlist_copy(const char **src, int len) {
  if (src != NULL) {
    int i;
    char ** copy = util_malloc(len * sizeof * copy , __func__);
    for (i=0; i < len; i++)
      copy[i] = util_alloc_string_copy(src[i]);
    return copy;
  } else 
    return NULL;
}


char * util_alloc_string_copy(const char *src ) {
  if (src != NULL) {
    char *copy = util_malloc((strlen(src) + 1) * sizeof *copy , __func__);
    strcpy(copy , src);
    return copy;
  } else 
    return NULL;
}




char * util_realloc_string_copy(char * old_string , const char *src ) {
  if (src != NULL) {
    char *copy = util_realloc(old_string , (strlen(src) + 1) * sizeof *copy , __func__);
    strcpy(copy , src);
    return copy;
  } else {
    if (old_string != NULL)
      free(old_string);
    return NULL;
  }
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



void util_free_string_list(char **list , int N) {
  int i;
  if (list != NULL) {
    for (i=0; i < N; i++) {
      if (list[i] != NULL)
	free(list[i]);
    }
    free(list);
  }
}


char ** util_alloc_string_list(int N, int len) {
  int i;
  char **list = calloc(N , sizeof *list);
  for (i=0; i < N; i++)
    list[i] = util_malloc(len , __func__);
  return list;
}


char * util_strcat_realloc(char *s1 , const char * s2) {
  if (s1 == NULL) 
    s1 = util_alloc_string_copy(s2);
  else {
    s1 = realloc(s1 , strlen(s1) + strlen(s2) + 1);
    strcat(s1 , s2);
  }
  return s1;
}


char * util_alloc_string_sum(const char ** string_list , int N) {
  int i , len;
  char * buffer;
  len = 0;
  for (i=0; i < N; i++) {
    if (string_list[i] != NULL)
      len += strlen(string_list[i]);
  }
  buffer = util_malloc(len + 1 , __func__);
  buffer[0] = '\0';
  for (i=0; i < N; i++) {
    if (string_list[i] != NULL)
      strcat(buffer , string_list[i]);
  }
  return buffer;
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
    
    fileList = util_malloc(files * sizeof *fileList , __func__);
    
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
  } else 
    util_abort("%s: failed to open directory: %s - aborting \n",__func__ , enspath);
}



/*****************************************************************/


/**
  Allocates a new string consisting of all the elements in item_list,
  joined together with sep as separator. Elements in item_list can be
  NULL, this will be replaced with the empty string.
*/


char * util_alloc_joined_string(const char ** item_list , int len , const char * sep) {
  if (len <= 0)
    return NULL;
  else {
    char * joined_string;
    int sep_length   = strlen(sep);
    int total_length = 0;
    int eff_len = 0;
    int i;
    for (i=0; i < len; i++) 
      if (item_list[i] != NULL) {
	total_length += strlen(item_list[i]);
	eff_len++;
      }

    if (eff_len > 0) {
      total_length += (eff_len - 1) * sep_length + 1;
      joined_string = util_malloc(total_length , __func__);
      joined_string[0] = '\0';
      for (i=0; i < len; i++) {
	if (item_list[i] != NULL) {
	  if (i > 0)
	    strcat(joined_string , sep);
	  strcat(joined_string , item_list[i]);
	}
      }
      return joined_string;
    } else
      return NULL;
  }
}


/**
  New string is allocated by joining the elements in item_list, with
  "\n" character as separator; an extra "\n" is also added at the end
  of the list.
*/
char * util_alloc_multiline_string(const char ** item_list , int len) {
  char * multiline_string = util_alloc_joined_string(item_list , len , UTIL_NEWLINE_STRING);
  multiline_string = util_realloc(multiline_string , (strlen(multiline_string) + strlen(UTIL_NEWLINE_STRING) + 1) * sizeof * multiline_string , __func__ );
  strcat(multiline_string , UTIL_NEWLINE_STRING);
  return multiline_string;
}


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
    offset += strspn(&line[offset] , sep);
  } while (line[offset] != '\0');

  if (tokens > 0) {
    token_list = util_malloc(tokens * sizeof * token_list , __func__);
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
      offset += strspn(&line[offset] , sep);
    } while (line[offset] != '\0');
  } else
    token_list = NULL;
  
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
    s = util_malloc(len + 1 , __func__);
    util_fread(s , 1 , len + 1 , stream , __func__);
  } 
  return s;
}


char * util_fread_realloc_string(char * old_s , FILE *stream) {
  int len;
  char *s = NULL;
  util_fread(&len , sizeof len , 1 , stream , __func__);
  if (len > 0) {
    s = util_realloc(old_s , len + 1 , __func__);
    util_fread(s , 1 , len + 1 , stream , __func__);
  } 
  return s;
}


void util_fskip_string(FILE *stream) {
  int len;
  util_fread(&len , sizeof len , 1 , stream , __func__);
  fseek(stream , len + 1 , SEEK_CUR);
}

void util_fwrite_int   (int value , FILE * stream)    { UTIL_FWRITE_SCALAR(value , stream); }
void util_fwrite_double(double value , FILE * stream) { UTIL_FWRITE_SCALAR(value , stream); }

void util_fwrite_int_vector   (const int * value    , int size , FILE * stream, const char * caller) { util_fwrite(value , sizeof * value, size , stream, caller); }
void util_fwrite_double_vector(const double * value , int size , FILE * stream, const char * caller) { util_fwrite(value , sizeof * value, size , stream, caller); }


int util_fread_int(FILE * stream) {
  int file_value;
  UTIL_FREAD_SCALAR(file_value , stream);
  return file_value;
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

void util_update_int_max_min(int value , int * max , int * min) {
  *min = util_int_min(value , *min);
  *max = util_int_max(value , *max);
}

void util_update_float_max_min(float value , float * max , float * min) {
  *min = util_float_min(value , *min);
  *max = util_float_max(value , *max);
}

void util_update_double_max_min(double value , double * max , double * min) {
  *min = util_double_min(value , *min);
  *max = util_double_max(value , *max);
}
  

void util_apply_double_limits(double * value , double min_value , double max_value) {
  if (*value < min_value)
    *value = min_value;
  else if (*value > max_value)
    *value = max_value;
}

void util_apply_float_limits(float * value , float min_value , float max_value) {
  if (*value < min_value)
    *value = min_value;
  else if (*value > max_value)
    *value = max_value;
}

void util_apply_int_limits(int * value , int min_value , int max_value) {
  if (*value < min_value)
    *value = min_value;
  else if (*value > max_value)
    *value = max_value;
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
    fprintf(stderr,"%s: current element size: %d \n",__func__ , element_size);
    util_abort("%s: can only endian flip 1/2/4/8 byte variables - aborting \n",__func__);
  }
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
      {
	char * cwd = util_alloc_cwd();
	fprintf(stderr,"%s: cwd:%s \n",__func__ , cwd);
	free(cwd);
      }
      if (abort_mode & ABORT_READ) util_abort("%s: ABORT_READ",__func__);
    }
  } else if (strcmp(mode ,"w") == 0) {
    stream = fopen(filename , "w");
    if (stream == NULL) {
      fprintf(stderr,"%s: failed to open:%s for writing: %s \n",__func__ , filename, strerror(errno));
      {
	char * cwd = util_alloc_cwd();
	fprintf(stderr,"%s: cwd:%s \n",__func__ , cwd);
	free(cwd);
      }
      if (abort_mode & ABORT_WRITE) util_abort("%s: ABORT_WRITE \n",__func__);
    }
  } else 
    util_abort("%s: open mode: '%s' not implemented - aborting \n",__func__ , mode);

  return stream;
}


FILE * util_fopen(const char * filename , const char * mode) {
  return util_fopen__(filename , mode , ABORT_READ + ABORT_WRITE);
}


void util_fwrite(const void *ptr , size_t element_size , size_t items, FILE * stream , const char * caller) {
  int items_written = fwrite(ptr , element_size , items , stream);
  if (items_written != items) 
    util_abort("%s/%s: only wrote %d/%d items to disk - aborting\n %s .\n",caller , __func__ , items_written , items , strerror(errno));
}


void util_fread(void *ptr , size_t element_size , size_t items, FILE * stream , const char * caller) {
  int items_read = fread(ptr , element_size , items , stream);
  if (items_read != items) 
    util_abort("%s/%s: only read %d/%d items from disk - aborting.\n %s \n",caller , __func__ , items_read , items , strerror(errno));
}

#undef ABORT_READ
#undef ABORT_WRITE


/*****************************************************************/

void * util_realloc(void * old_ptr , size_t new_size , const char * caller) {
  void * tmp = realloc(old_ptr , new_size);
  if ((tmp == NULL) && (new_size > 0)) 
    util_abort("%s: failed to realloc %d bytes - aborting \n",caller , new_size);
  
  return tmp;
}


void * util_malloc(size_t size , const char * caller) {
  void *data = malloc(size);
  if (data == NULL) 
    util_abort("%s: failed to allocate %d bytes - aborting \n",caller , size);

  /* 
     Initializing with something different from zero - hopefully
     errors will pop up more easily this way?
  */
  memset(data , 255 , size);
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


void util_read_filename(const char * prompt , int prompt_len , bool must_exist , char * filename) {
  bool ok = false;
  while (!ok) {
    util_read_string(prompt , prompt_len , filename);
    if (must_exist)
      ok = util_file_exists(filename);
    else
      ok = true;
    if (!ok) 
      fprintf(stderr,"File: %s does not exist - try again.\n",filename);
  }
}


/*****************************************************************/

/**
  This function reads data from the input pointer data, and writes a
  compressed copy into to the target buffer zbuffer. On input
  data_size should be the *number of bytes* in data compressed_size
  should be a reference to the size (in bytes) of zbuffer, on return
  this has been updated to reflect the new compressed size.
*/
void util_compress_buffer(const void * data , int data_size , void * zbuffer , unsigned long * compressed_size) {
  int compress_result;
  compress_result = compress(zbuffer , compressed_size , data , data_size);
  if (compress_result != Z_OK) 
    util_abort("%s: returned %d - different from Z_OK - aborting\n",__func__ , compress_result);
}



/**
  This function allocates a new buffer which is a compressed version
  of the input buffer data. The input variable data_size, and the
  output * compressed_size are the size - *in bytes* - of input and
  output.
*/
void * util_alloc_compressed_buffer(const void * data , int data_size , unsigned long * compressed_size) {
  void * zbuffer = util_malloc(data_size , __func__);
  *compressed_size = data_size;
  util_compress_buffer(data , data_size , zbuffer , compressed_size);
  zbuffer = util_realloc(zbuffer , *compressed_size , __func__ );
  return zbuffer;
}


/*
Layout on disk when using util_fwrite_compressed:

  /-------------------------------
  |uncompressed total size
  |size of compression buffer
  |----
  |compressed size
  |compressed block
  |current uncompressed offset
  |....
  |compressed size
  |compressed block
  |current uncompressed offset
  |....
  |compressed size
  |compressed block
  |current uncompressed offset
  \------------------------------
*/

void util_fwrite_compressed(const void * _data , int size , FILE * stream) {
  if (size == 0) {
    fwrite(&size , sizeof size , 1 , stream);
    return;
  }
  {
    const char * data = (const char *) _data;
    const int max_buffer_size      = 128 * 1048580; /* 128 MB */
    int       required_buffer_size = (int) ceil(size * 1.001 + 12);
    int       buffer_size , block_size;
    void * zbuffer;
    
    buffer_size = util_int_min(required_buffer_size , max_buffer_size);
    do {
      zbuffer = malloc(buffer_size);
      if (zbuffer == NULL)
	buffer_size /= 2;
    } while(zbuffer == NULL);
    block_size = (int) (floor(buffer_size / 1.002) - 12);
    
    fwrite(&size        , sizeof size        , 1 , stream);
    fwrite(&buffer_size , sizeof buffer_size , 1 , stream);
    
    {
      int offset = 0;
      do {
	unsigned long compressed_size = buffer_size;
	int this_block_size           = util_int_min(block_size , size - offset);
	util_compress_buffer(&data[offset] , this_block_size , zbuffer , &compressed_size);
	fwrite(&compressed_size , sizeof compressed_size , 1 , stream);
	{
	  int bytes_written = fwrite(zbuffer , 1 , compressed_size , stream);
	  if (bytes_written < compressed_size) 
	    util_abort("%s: wrote only %d/%ld bytes to compressed file  - aborting \n",__func__ , bytes_written , compressed_size);

	}
	offset += this_block_size;
      fwrite(&offset , sizeof offset , 1 , stream);
      } while (offset < size);
    }
    free(zbuffer);
  }
}



void util_fread_compressed(char *data , FILE * stream) {
  int size , offset;
  int buffer_size;
  void * zbuffer;

  fread(&size        , sizeof size        , 1 , stream); 
  if (size == 0) return;


  fread(&buffer_size , sizeof buffer_size , 1 , stream);
  zbuffer = util_malloc(buffer_size , __func__);
  offset = 0;
  do {
    unsigned long compressed_size;
    int block_size = size - offset;
    int compress_result;
    fread(&compressed_size , sizeof compressed_size , 1 , stream);
    {
      int bytes_read = fread(zbuffer , 1 , compressed_size , stream);
      if (bytes_read < compressed_size) 
	util_abort("%s: read only %d/%d bytes from compressed file - aborting \n",__func__ , bytes_read , compressed_size);
      
    }
    compress_result = uncompress(&data[offset] , &block_size , zbuffer , compressed_size);
    if (compress_result != Z_OK) 
      util_abort("%s: uncompress returned %d - aborting \n",__func__ , compress_result);
    
    offset += block_size;
    {
      int file_offset;
      fread(&file_offset , sizeof offset , 1 , stream); 
      if (file_offset != offset) 
	util_abort("%s: something wrong when reding compressed stream - aborting \n",__func__);
    }
  } while (offset < size);
  free(zbuffer);
}


void util_fskip_compressed(FILE * stream) {
  int size , offset;
  int buffer_size;
  fread(&size        , sizeof size        , 1 , stream);
  if (size == 0) return;

  
  fread(&buffer_size , sizeof buffer_size , 1 , stream);
  do {
    unsigned long compressed_size;
    fread(&compressed_size , sizeof compressed_size , 1 , stream);
    fseek(stream  , compressed_size , SEEK_CUR);
    fread(&offset , sizeof offset , 1 , stream);
  } while (offset < size);
}


/** 
    This is the core filtering function. Based on this we can do the following filtering:

    buffer -> existing buffer (can fail if target buffer is to small).
    buffer -> newly allocated buffer
    buffer -> file
    file   -> file
    
*/


/*
static bool util_filter_buffer(const char * src_buffer , const char * target_file , char ** _target_buffer, int * _target_buffer_size , bool target_buffer_can_resize , char start_char , char end_char , const hash_type * kw_hash , util_filter_warn_type warning_mode) {
  return true;
}
*/



/**
This funtion does template substitution. It scans through the src_file
looking for keywords. The keywords are stored in a hash_table as
void_arg instances, that way the function can substitute
int/double/char * /... values reasonably transparent. 

The keywords which are replaced with values, are supposed to be
bracketed with the two characters start_char and end_char; if comment
!= NULL comments in src_file are stripped out.

There are two types of situtations where warnings could be interesting:

 o If the src_file contains a keyword <ABC> (assuming start_char and
   end_char equal '<' and '>' respectively), but the kw_hash table
   does *NOT* have the "ABC" keyword, the string <ABC> is passed
   through literally.

 o The opposite situtation, the table contains a certain keyword "XYZ"
   which is not found in the template file.

The warning_mode variable can have the following values:

 util_filter_warn0        : No warning will be issued in either of the two cases.
 util_filter_warn_unknown : A warning will be issued in the first case.
 util_filter_warn_unused  : A warning will be issued in the first case.
 util_filter_warn_all     : A warning will be issued in both cases.

*/

void util_filter_file(const char * src_file , const char * comment , const char * target_file , char start_char , char end_char , const hash_type * kw_hash , util_filter_warn_type warning_mode) {
  set_type * used_set;
  int    index, buffer_size;
  char * buffer = util_fread_alloc_file_content(src_file , comment , &buffer_size);
  FILE * stream = util_fopen(target_file , "w");
  char * kw     = NULL;
  

  if ((warning_mode & util_filter_warn_unused) != 0) {
    char ** key_list = hash_alloc_keylist(kw_hash);
    used_set = set_alloc(hash_get_size(kw_hash) , (const char **) key_list);
    hash_free_ext_keylist(kw_hash , key_list);
  }

  index = 0;
  while (index < buffer_size) {
    if (buffer[index] == start_char) {
      int start_pos = index;
      int end_pos;
      index++;
      while (buffer[index] != end_char && !EOL_CHAR(buffer[index]) && index < buffer_size && buffer[index] != ' ') 
	index++;
      end_pos = index; 
      {
	bool write_src = true;
	
	if (buffer[index] == end_char) {
	  if (end_pos - start_pos > 1) {
	    kw = util_realloc_substring_copy(kw , &buffer[start_pos + 1] , end_pos - start_pos - 1);
            if (hash_has_key(kw_hash , kw)) {
	      void_arg_fprintf_typed(hash_get(kw_hash , kw) , stream);
	      write_src = false;
	      index++;
	      if ((warning_mode & util_filter_warn_unused) != 0) 
		if (set_has_key(used_set , kw))
		  set_remove_key(used_set , kw);
	    } else 
	      if ((warning_mode & util_filter_warn_unknown) != 0) 
		fprintf(stderr," ** Warning ** no defintion for keyword:%s \n",kw);
	  }
	}	
	if (write_src)
	  fwrite(&buffer[start_pos] , 1 , end_pos - start_pos , stream);
      }
    } else {
      fputc(buffer[index] , stream);
      index++;
    }
  }
  fclose(stream);
  free(kw);
  free(buffer);

  if (((warning_mode & util_filter_warn_unused) != 0) && (set_get_size(used_set) > 0)) {
    fprintf(stderr," ** Warning the following keys were not used when filtering \"%s -> %s\": (",src_file, target_file);
    set_fprintf(used_set , stderr);
    fprintf(stderr,")\n");
    set_free(used_set);
  }
}


/**
  This function uses the external program addr2line to convert the
  hexadecimal adress given by the libc function backtrace() into a
  function name and file:line.

  Observe that the function is quite involved, so if util_abort() is
  called because something is seriously broken, it might very well fail.

  The executable should be found from one line in the backtrace with
  the function util_bt_alloc_current_executable(), the argument
  bt_symbol is the lines generated by the  bt_symbols() function.

  This function is purely a helper function for util_abort().
*/

static void util_addr2line_lookup(const char * executable , const char * bt_symbol , char ** func_name , char ** file_line) {
  const char *tmp_file = "/tmp/addr2line";
  char * adress;
  char * cmd;
  {
    int start_pos = 0;
    int end_pos;   
    while ( bt_symbol[start_pos] != '[')
      start_pos++;

    end_pos = start_pos;
    while ( bt_symbol[end_pos] != ']') 
      end_pos++;
    
    adress = util_alloc_substring_copy( &bt_symbol[start_pos + 1] , end_pos - start_pos - 1 );
  }
  
  cmd = util_alloc_joined_string((const char *[6]) {"addr2line --functions --exe=" , executable , " " , adress , " > " , tmp_file} , 6 , "");
  system(cmd);
  {
    bool at_eof;
    FILE * stream = util_fopen(tmp_file , "r");
    *func_name = util_fscanf_alloc_line(stream , &at_eof);
    *file_line = util_fscanf_alloc_line(stream , &at_eof);
    fclose(stream);
  }
  free(adress);
  free(cmd);
}



/**
  This function takes one string from the string list generated by
  bt_symbols(). From this string it extracts the full path to the
  current executable. This path is needed for subsequent calls to
  util_addr2line_lookup().

  This function is purely a helper function for util_abort().
*/

static char * util_bt_alloc_current_executable(const char * bt_symbol) {
  int paren_pos = 0;
  char * path;
  while (bt_symbol[paren_pos] != '(' && bt_symbol[paren_pos] != ' ')
    paren_pos++;
  
  path = util_alloc_substring_copy(bt_symbol , paren_pos);
  if (util_is_abs_path(path))
    return path;
  else {
    char * full_path = util_alloc_full_path(getenv("PWD") , path);
    free(path);
    return full_path;
  }
}


/**
  This function prints a message to stderr and aborts. The function is
  implemented with the help of a variable length argument list - just
  like printf(fmt , arg1, arg2 , arg3 ...);

  A backtrace is also included, with the help of the exernal utility
  addr2line, this backtrace is converted into usable
  function/file/line information (provided the required debugging
  information is compiled in).
*/


void util_abort(const char * fmt , ...) {
  const bool include_backtrace = true;
  va_list ap;
  va_start(ap , fmt);
  vfprintf(stderr , fmt , ap);
  va_end(ap);
  if (include_backtrace) {
    const int max_bt = 50;
    char *executable;
    void *array[max_bt];
    char **strings;
    char ** func_list;
    char ** file_line_list;
    int    max_func_length = 0;
    int    size,i;

    size       = backtrace(array , max_bt);
    strings    = backtrace_symbols(array , size);
    executable = util_bt_alloc_current_executable(strings[0]);
    printf("Parsing executable:%s -> <%s> \n",strings[0] , executable);

    size -= 2; /* The two last symbols in the backtrace are libc functions which are not available anyway. */ 
    func_list      = util_malloc(size * sizeof * func_list      , __func__);
    file_line_list = util_malloc(size * sizeof * file_line_list , __func__);
    
    for (i=0; i < size; i++) {
      util_addr2line_lookup(executable , strings[i] , &func_list[i] , &file_line_list[i]);
      max_func_length = util_int_max(max_func_length , strlen(func_list[i]));
    }

    {
      char fmt[64];
      sprintf(fmt, " #%s02d %s-%ds(..) in %ss \n" , "%" , "%" , max_func_length , "%");
      fprintf(stderr , "--------------------------------------------------------------------------------\n");
      for (i=0; i < size; i++) 
	fprintf(stderr, fmt , i , func_list[i], file_line_list[i]);
      fprintf(stderr , "--------------------------------------------------------------------------------\n");
      util_free_string_list(func_list      , size);
      util_free_string_list(file_line_list , size);
    }
    free(strings);
    free(executable);
  }
  
  abort();
}



/** 
    This funny function is used to block execution while a file is
    growing. It is a completely heuristic attempt to ensure that the
    writing of a certain file is finished before execution continues.
*/

void util_block_growing_file(const char * file) {
  const int usleep_time = 250; 
  int prev_size;
  int current_size = 0;
  do {
    prev_size = current_size;
    usleep(usleep_time);
    current_size = util_file_size(file);
  } while (current_size != prev_size);
}

/**
   This is much similar to the preivious function, this function
   blocks until the number of entries in directory is constant (it
   does not consider wether the size of the files is changing, again
   this is quite special-purpose function for the enkf + ECLIPSE cooperation.
*/ 

void util_block_growing_directory(const char * directory) {
  const int usleep_time = 250; 
  int prev_size;
  int current_size = 0;
  do {
    prev_size  = current_size;
    usleep(usleep_time);
    current_size = 0;
    {
      DIR * dirH  = opendir( directory );
      struct dirent * dentry;
      while ( (dentry = readdir(dirH)) != NULL) 
	current_size++;

      closedir(dirH);
    }
  } while (current_size != prev_size);
}



#include "util_path.c"

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
  
