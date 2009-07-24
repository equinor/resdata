/**
  This file contains a large number of utility functions for memory
  handling, string handling and file handling. Observe that all these
  functions are just that - functions - there is no associated state
  with any of these functions.
  
  The file util_path.c is included in this, and contains path
  manipulation functions which explicitly use the PATH_SEP variable.
*/

#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <zlib.h>
#include <math.h>
#include <stdarg.h>
#include <execinfo.h>
#include <pthread.h>
#include <util.h>


#define FLIP16(var) (((var >> 8) & 0x00ff) | ((var << 8) & 0xff00))

#define FLIP32(var) (( (var >> 24) & 0x000000ff) | \
		      ((var >>  8) & 0x0000ff00) | \
		      ((var <<  8) & 0x00ff0000) | \
		      ((var << 24) & 0xff000000))

#define FLIP64(var)  (((var >> 56)  & 0x00000000000000ff)| \
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

#undef strncpy // This is for some reason needed in RH3


LIBRARY_VERSION(libutil)

void util_fread_dev_random(int buffer_size , char * buffer) {
  FILE * stream = util_fopen("/dev/random" , "r");
  if (fread(buffer , 1 , buffer_size , stream) != buffer_size)
    util_abort("%s: failed to read:%d bytes from /dev/random \n",__func__ , buffer_size);
  
  fclose(stream);
}


void util_fread_dev_urandom(int buffer_size , char * buffer) {
  FILE * stream = util_fopen("/dev/urandom" , "r");
  if (fread(buffer , 1 , buffer_size , stream) != buffer_size)
    util_abort("%s: failed to read:%d bytes from /dev/random \n",__func__ , buffer_size);
  
  fclose(stream);
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




/**
   The input string is freed, and a new storage
   without quotes is returned. 
*/
char * util_realloc_dequoted_string(char *s) {
  char first_char = s[0];
  char last_char  = s[strlen(s) - 1];
  char *new;
  int offset , len;

  if ((first_char == '\'') || (first_char == '\"'))
    offset = 1;
  else 
    offset = 0;
  
  if ((last_char == '\'') || (last_char == '\"'))
    len = strlen(s) - offset - 1;
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


char * util_alloc_strupr_copy(const char * s) {
  char * c = util_alloc_string_copy(s);
  util_strupr(c);
  return c;
}


/** 
    Replaces all occurences of c1 in s with c2.
*/
void util_string_tr(char * s, char c1, char c2) {
  int i;
  for (i=0; i < strlen(s);i++)
    if (s[i] == c1) s[i] = c2;
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



/**
   This function will reposition the stream pointer at the the first
   occurence of 'string'. If 'string' is found the function will
   return true, otherwise the function will return false, and stream
   pointer will be at the original position.

   If skip_string == true the stream position will be positioned
   immediately after the 'string', otherwise it will be positioned at
   the beginning of 'string'.
*/

bool util_fseek_string(FILE * stream , const char * string , bool skip_string) {
  int len              = strlen( string );
  long int initial_pos = ftell( stream );   /* Store the inital position. */
  bool string_found    = false;
  bool cont            = true;
  do {
    int c = fgetc( stream );
    if (c == string[0]) {  /* OK - we got the first character right - lets try in more detail: */
      long int current_pos  = ftell(stream);
      bool     equal        = true;

      for (int string_index = 1; string_index < len; string_index++) {
        c = fgetc( stream );
        if (c != string[string_index]) {
          equal = false;
          break;
        }
      }

      if (equal) {
        string_found = true;
        cont = false;
      } else /* Go back to current pos and continue searching. */
        fseek(stream , current_pos , SEEK_SET);
      
    }
    if (c == EOF) 
      cont = false;
  } while (cont);
  
  
  if (string_found) {
    if (!skip_string)
      fseek(stream , -strlen(string) , SEEK_CUR); /* Reposition to the beginning of 'string' */
  } else
    fseek(stream , initial_pos , SEEK_SET);       /* Could not find the string reposition at initial position. */
  
  return string_found;
}



/**
  This function will allocate a character buffer, and read file
  content all the way up to 'stop_string'. If the stop_string is not
  found, the function will return NULL, and the file pointer will be
  unchanged.

  If include_stop_string is true the returned string will end with
  stop_string, and the file pointer will be positioned right AFTER
  stop_string, otherwise the file_pointer will be positioned right
  before stop_string.
*/



char * util_fscanf_alloc_upto(FILE * stream , const char * stop_string, bool include_stop_string) {
  long int start_pos = ftell(stream);
  if (util_fseek_string(stream , stop_string , include_stop_string)) {
    long int end_pos = ftell(stream);
    int      len     = end_pos - start_pos;
    char * buffer    = util_malloc( (len + 1) * sizeof * buffer , __func__);

    fseek(stream , start_pos , SEEK_SET);
    util_fread( buffer , 1 , len , stream , __func__);
    buffer[len] = '\0';
    
    return buffer;
  } else
    return NULL;   /* stop_string not found */
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
  
  if (fseek(stream , init_pos , SEEK_SET) != 0) 
    util_abort("%s: fseek failed: %d/%s \n",__func__ , errno , strerror(errno));

  new_line = util_realloc(line , len + 1 , __func__);
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
  
  if (new_line != NULL) {
    char * strip_line = util_alloc_strip_copy(new_line);
    free(new_line);
    
    return strip_line;
  } else
    return NULL;
}


char * util_fscanf_alloc_line(FILE *stream , bool *at_eof) {
  return util_fscanf_alloc_line__(stream , at_eof , NULL);
}


char * util_fscanf_realloc_line(FILE *stream , bool *at_eof , char *line) {
  return util_fscanf_alloc_line__(stream , at_eof , line);
}



/**
   Reads characters from stdin until EOL/EOF is detected. A '\0' is
   appended to the resulting string before it is returned. If the
   function reads an immediate EOF/EOL, i.e. the user enters an empty
   input string, NULL (and not "") is returned.

   Observe that is this function does *not* cooperate very nicely with
   fscanf() based input, because fscanf will leave a EOL character in
   the input buffer, which will lead to immediate return from this
   function. Hence if this function is called after a fsacnf() based
   function it is essential to preceede this function with one call to
   getchar() to clear the EOL character.
*/

char * util_alloc_stdin_line() {
  int input_size = 256;
  char * input   = util_malloc(input_size , __func__);
  int index = 0;
  bool end = false;
  int c;
  do {
    c = getchar();
    if ((!EOL_CHAR(c)) && (c != EOF)) {
      input[index] = c;
      index++;
      if (index == (input_size - 1)) { /* Reserve space for terminating \0 */
	input_size *= 2;
	input = util_realloc(input , input_size , __func__);
      }
    } else end = true;
  } while (!end);
  if (index == 0) { 
    free(input);
    input = NULL;
  } else {
    input[index] = '\0';  
    input = util_realloc(input , strlen(input) + 1 , __func__);
  }
  
  return input;
}



char * util_realloc_stdin_line(char * p) {
  util_safe_free(p);
  return util_alloc_stdin_line();
}



/**
   This function will allocate and read a line from stdin. If there is
   no input waiting on stdin (this typically only applies if stdin is
   redirected from a file/PIPE), the function will sleep for 'usec'
   microseconds and try again.
*/

char * util_blocking_alloc_stdin_line(unsigned long usec) {
  char * line;
  do {
    line = util_alloc_stdin_line();
    if (line == NULL) 
      usleep(usec);
  } while (line == NULL);
  return line;
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
    *t = util_make_date(day , month , year );  
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
  *at_eof     = false;
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
  bool cont     = true;
  do {
    int c = fgetc(stream);
    if (c == EOF) {
      *at_eof = true;
      cont = false;
    } else {
      if (strchr(skip_set , c) == NULL) {
	/* c is not in skip_set */
	if (!complimentary_set)
	  cont = false;
      } else {
	/* c is in skip_set */
	if (complimentary_set)
	  cont = false;
      }
    }
  } while (cont);
  if (!*at_eof)
    fseek(stream , -1 , SEEK_CUR);
}


void util_fskip_chars(FILE * stream , const char * skip_set , bool *at_eof) {
  util_fskip_chars__(stream , skip_set , false , at_eof);
}

void util_fskip_cchars(FILE * stream , const char * skip_set , bool *at_eof) {
  util_fskip_chars__(stream , skip_set , true , at_eof);
}


static void util_fskip_space__(FILE * stream , bool complimentary_set , bool *at_eof) {
  bool cont     = true;
  do {
    int c = fgetc(stream);
    if (c == EOF) {
      *at_eof = true;
      cont = false;
    } else {
      if (!isspace(c)) {
	/* c is not in space set. */
	if (!complimentary_set)
	  cont = false;
      } else {
	/* c is in skip_set */
	if (complimentary_set)
	  cont = false;
      }
    }
  } while (cont);
  if (!*at_eof)
    fseek(stream , -1 , SEEK_CUR);
}


void util_fskip_space(FILE * stream ,  bool *at_eof) {
  util_fskip_space__(stream , false , at_eof);
}




/** 
    This functions reads a token[1] from stream, allocates storage for
    the it, and returns the newly allocated storage. Observe that the
    function does *NOT* read past end of line. If no token can be the
    function will return NULL.

    Example:
    --------

    File:
     ________________
    /
    | This is a file
    | Line2
    | Line3
    \________________


   bool at_eof = 0;
   char * token;
   while (!at_eof) {
      token = util_fscanf_alloc_token(stream);
      if (token != NULL) {
         printf("Have read token:%s \n",token);  
         free(token);
      } else {
         printf("Have reached EOL/EOF \n");
         util_forward_line(stream , &at_eof);
      }
   }

   This will produce the output:
   Have read token: This
   Have read token: is
   Have read token: a 
   Have read token: file
   Have reached EOL/EOF
   Have read token: Line2
   Have reached EOL/EOF
   Have read token: Line3
   Have reached EOL/EOF


   [1]: A token is defined as a sequence of characters separated by
    white-space or tab.
*/
    



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



/**
  This function parses a string literal (hopfully) containing a
  represantation of a double. The return value is true|false depending
  on the success of the parse operation, the parsed value is returned
  by reference.


  Example:
  --------
  const char * s = "78.92"
  double value;
  
  if (util_sscanf_double(s , &value)) 
    printf("%s is a valid double\n");
  else
    printf("%s is NOT a valid double\n");

*/
 
bool util_sscanf_double(const char * buffer , double * value) { 
  bool value_OK = false; 
  char * error_ptr;

  double tmp_value = strtod(buffer , &error_ptr);
  /*
    Skip trailing white-space
  */
  while (error_ptr[0] != '\0' && isspace(error_ptr[0]))
    error_ptr++;
  
  if (error_ptr[0] == '\0') {
    value_OK = true; 
    if (value != NULL)
      *value = tmp_value;
  } 
  return value_OK;
}



/**
   Takes a char buffer as input, and parses it as an integer. Returns
   true if the parsing succeeded, and false otherwise. If parsing
   succeded, the integer value is returned by reference.
*/


bool util_sscanf_int(const char * buffer , int * value) {
  bool value_OK = false;
  char * error_ptr;

  int tmp_value = strtol(buffer , &error_ptr , 10);

  /*
    Skip trailing white-space
  */

  while (error_ptr[0] != '\0' && isspace(error_ptr[0]))
    error_ptr++;

  if (error_ptr[0] == '\0') {
    value_OK = true; 
    if (value != NULL)
      *value = tmp_value;
  } 
  return value_OK;
}




/**
   This function parses the string 's' for an integer. The return
   value is a pointer to the first character which is not an integer
   literal, or NULL if the whole string is read.

   The actual integer value is returned by reference. In addition a
   bool 'OK' is returned by reference, observe that that the bool OK
   is checked on function entry, and must point to true then.

   The somewhat contrived interface is to facilitate repeated calls on
   the same string to get out all the integers, typically to be used
   together with util_skip_sep().

   Example
   -------

   s = "1, 10, 78, 67";
         |   |   |   NULL
         1   2   3   3
         
   The vertical bars indicate the return values.	 

*/

  
const char * util_parse_int(const char * s , int * value, bool *OK) {
  if (*OK) {
    char * error_ptr;
    *value = strtol(s , &error_ptr , 10);
    if (error_ptr == s) *OK = false;
    return error_ptr;
  } else
    return NULL;
}



/**
   This function will skip the characters in s which are in the string
   'sep_set' and return a pointer to the first character NOT in
   'sep_set'; this is basically strspn() functionality. But it will
   update a reference bool OK if no characters are skipped -
   i.e. there should be some characters to skip. Typically used
   together with util_parse_int():


   Example
   -------

   const char * s = "1, 6 , 79 , 89 , 782";
   const char * current_ptr = s;
   bool OK = true;
   while (OK) {
      int value;
      current_ptr = util_parse_int(current_ptr , &value , &OK);
      if (OK) 
         printf("Found:%d \n",value);
      current_ptr = util_skip_sep(current_ptr , " ," , &OK);
   }

   
*/

const char * util_skip_sep(const char * s, const char * sep_set, bool *OK) {
  if (*OK) {
    int sep_length = strspn(s , sep_set);
    if (sep_length == 0)
      *OK = false;
    return &s[sep_length];
  } else 
    return NULL;
}





/**
   This function will parse string containing an integer, and an
   optional suffix. The valid suffizes are KB,MB and GB (any case is
   allowed); if no suffix is appended the buffer is assumed to contain
   a memory size already specified in bytes.
   

   Observe that __this_function__ allows an arbitrary number of spaces
   between between the integer literal and the suffix string; however
   this might be tricky when parsing. It is probably best to disallow
   these spaces?

   "1GB", "1 GB", "1    gB"

   are all legitimate. The universal factor used is 1024:

      KB => *= 1024
      MB => *= 1024 * 1024;
      GB => *= 1024 * 1024 * 1024;
      
   Observe that if the functions fails to parse/interpret the string
   it will return false, and set the reference value to 0. However it
   will not fail with an abort. Overflows are *NOT* checked for.
*/


bool util_sscanf_bytesize(const char * buffer, size_t *size) {
  size_t value;
  char * suffix_ptr;
  size_t KB_factor = 1024;
  size_t MB_factor = 1024 * 1024;
  size_t GB_factor = 1024 * 1024 * 1024;
  size_t factor    = 1;
  bool   parse_OK  = true;

  value = strtol(buffer , &suffix_ptr , 10);
  if (suffix_ptr[0] != '\0') {
    while (isspace(suffix_ptr[0])) 
      suffix_ptr++;
    {
      char * upper = util_alloc_string_copy(suffix_ptr);
      if (strcmp(upper,"KB") == 0)
	factor = KB_factor;
      else if (strcmp(upper,"MB") == 0)
	factor = MB_factor;
      else if (strcmp(upper , "GB") == 0)
	factor = GB_factor;
      else
	parse_OK = false;
      /* else - failed to parse - returning false. */
      free(upper);
    }
  } 

  if (size != NULL) {
    if (parse_OK)
      *size = value * factor;
    else
      *size = 0;
  }
  
  return parse_OK;
}



/** 
    Succesfully parses:

      1 , T (not 't') , True (with any case) => true
      0 , F (not 'f') , False(with any case) => false
      
    Else the parsing fails.
*/


bool util_sscanf_bool(const char * buffer , bool * _value) {
  bool parse_OK = false;
  bool value    = false; /* Compiler shut up */

  if (strcmp(buffer,"1") == 0) {
    parse_OK = true;
    value = true;
  } else if (strcmp(buffer , "0") == 0) {
    parse_OK = true;
    value = false;
  } else if (strcmp(buffer , "T") == 0) {
    parse_OK = true;
    value = true;
  } else if (strcmp(buffer , "F") == 0) {
    parse_OK = true;
    value = false;
  } else {
    char * local_buffer = util_alloc_string_copy(buffer);
    util_strupr(local_buffer);
    
    if (strcmp(local_buffer , "TRUE") == 0) {
      parse_OK = true;
      value = true;
    } else if (strcmp(local_buffer , "FALSE") == 0) {
      parse_OK = true;
      value = false;
    } 

    free(local_buffer);
  }
  if (_value != NULL)
    *_value = value;
  return parse_OK;
}


/**
   Takes a stream as input. Reads one string token from the stream,
   and tries to interpret the token as an integer with the function
   util_sscanf_int(). Returns true if the parsing succeeded, and false
   otherwise. If parsing succeded, the integer value is returned by
   reference.

   If the parsing fails the stream is repositioned at the location it
   had on entry to the function.
*/
   

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


/**
   Prompt .........====>
   <-------1------><-2-> 

   The section marked with 1 above is the prompt length, i.e. the
   input prompt is padded wth one blank, and then padded with
   'fill_char' (in the case above that is '.') characters up to a
   total length of prompt_len. Then the the termination string ("===>"
   above) is added. Observe the following:

   * A space is _always_ added after the prompt, before the fill char
     comes, even if the prompt is too long in the first place.

   * No space is added at the end of the termination string. If
     you want a space, that should be included in the termination
     string.

*/


void util_printf_prompt(const char * prompt , int prompt_len, char fill_char , const char * termination) {
  int current_len = strlen(prompt) + 1;
  printf("%s ",prompt);  /* Observe that one ' ' is forced in here. */ 
  
  while (current_len < prompt_len) {
    fputc(fill_char , stdout);
    current_len++;
  }
  printf("%s" , termination);

}


/**
   This functions presents the user with a prompt, and reads an
   integer - the integer value is returned. The functions will loop
   indefinitely until a valid integer is entered.
*/

int util_scanf_int(const char * prompt , int prompt_len) {
  char input[256];
  int  int_value;
  bool OK;
  do {
    util_printf_prompt(prompt , prompt_len, '=', "=> ");
    scanf("%s" , input);
    OK = util_sscanf_int(input , &int_value);
  } while (!OK);
  getchar(); /* eating a \r left in the stdin input buffer. */
  return int_value;
}


double util_scanf_double(const char * prompt , int prompt_len) {
  char input[256];
  double  double_value;
  bool OK;
  do {
    util_printf_prompt(prompt , prompt_len, '=', "=> ");
    scanf("%s" , input);
    OK = util_sscanf_double(input , &double_value);
  } while (!OK);
  getchar(); /* eating a \r left in the stdin input buffer. */
  return double_value;
}


/** 
    The limits are inclusive.
*/
int util_scanf_int_with_limits(const char * prompt , int prompt_len , int min_value , int max_value) {
  int value;
  char * new_prompt = util_alloc_sprintf("%s [%d:%d]" , prompt , min_value , max_value);
  do {
    value = util_scanf_int(new_prompt , prompt_len);
  } while (value < min_value || value > max_value);
  free(new_prompt);
  return value;
}




char * util_scanf_alloc_string(const char * prompt) {
  char input[256];
  printf("%s" , prompt);
  scanf("%256s" , input);
  return util_alloc_string_copy(input);
}



/**
   This function counts the number of lines from the current position
   in the file, to the end of line. The file pointer is repositioned
   at the end of the counting.
*/


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


/** 
    buffer_size is _only_ for a return (by reference) of the size of
    the allocation. Can pass in NULL if that size is not interesting.
*/

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
    
    if(target_buffer[target_pos-1] == EOF)
        target_buffer[target_pos-1] = '\0';
    else 
        target_buffer[target_pos] = '\0';

    free(src_buffer);
    
    target_buffer = util_realloc(target_buffer , strlen(target_buffer) + 1 , __func__);
    if (buffer_size != NULL) *buffer_size  = strlen(target_buffer);
    return target_buffer;
  } else {
    if (buffer_size != NULL) *buffer_size = file_size;
    
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
      util_abort("%s: not all bytes written to target stream - aborting \n",__func__);
  }

}


static void util_copy_file__(const char * src_file , const char * target_file, int buffer_size , void * buffer) {
  if (util_same_file(src_file , target_file)) 
    fprintf(stderr,"%s Warning: trying to copy %s onto itself - noting done\n",__func__ , src_file);
  else {
    {
      FILE * src_stream      = util_fopen(src_file     , "r");
      FILE * target_stream   = util_fopen(target_file  , "w");
      
      util_copy_stream(src_stream , target_stream , buffer_size , buffer);
      fclose(src_stream);
      fclose(target_stream);
    }
  }
}



void util_copy_file(const char * src_file , const char * target_file) {
  void * buffer   = NULL;
  int buffer_size = util_int_max( 32 , util_file_size(src_file) );  /* The copy stream function will hang if buffer size == 0 */
  do {
    buffer = malloc(buffer_size);
    if (buffer == NULL) buffer_size /= 2;
  } while ((buffer == NULL) && (buffer_size > 0));
  
  if (buffer_size == 0)
    util_abort("%s: failed to allocate any memory ?? \n",__func__);
  
  util_copy_file__(src_file , target_file , buffer_size , buffer);
  free(buffer);
}



/**
   Externals:
*/
typedef struct msg_struct msg_type;
msg_type   * msg_alloc(const char *);
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
	      util_copy_file__( full_src_path , full_target_path , buffer_size , buffer);
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
      msg = msg_alloc(prompt);
      msg_show( msg );
      util_copy_directory__(src_path , target_path , buffer_size , buffer ,msg);
    }
    msg_free(msg , true);
  }
  free( buffer );
  free(target_path);
  util_free_stringlist( path_parts , num_components );
}



bool util_file_exists(const char *filename) {
  struct stat stat_buffer;
  int stat_return = stat(filename , &stat_buffer);
  if (stat_return == 0)
    return true;
  else {
    if (errno == ENOENT)
      return false;
    else {
      util_abort("%s: error checking for file:%s  %d/%s \n",__func__ , filename , errno , strerror(errno));
      return false;
    }
  }
}


/**
   This function will start at 'root_path' and then recursively go
   through all file/subdirectore located below root_path. For each
   file in this tree it will call the user-supplied funtion
   'file_callback'. The arguments to file_callback will be:
   (root_path, file ,callback_arg):

   Example
   -------
   Root
   Root/File1
   Root/File2
   Root/dir
   Root/dir/fileXX

   The call:
      util_walk_directory("Root" , callback , arg);
      
   Will result in the following calls to the callback:

      callback("Root" , "File1" , arg); 
      callback("Root" , "File2" , arg); 
      callback("Root/dir" , "fileXX" , arg); 

   Symlinks are ignored when descending into subdirectories.     
*/


void util_walk_directory(const char * root_path , file_callback_ftype * file_callback , void * callback_arg) {
  {
    DIR * dirH = opendir( root_path );
    if (dirH == NULL) 
      util_abort("%s: failed to open directory:%s / %s \n",__func__ , root_path , strerror(errno));

    {
      struct dirent * dp;
      do {
	dp = readdir(dirH);
	if (dp != NULL) {
	  if (dp->d_name[0] != '.') {
	    char * full_path    = util_alloc_filename(root_path , dp->d_name , NULL);

	    if (util_is_file( full_path )) 
	      file_callback( root_path , dp->d_name , callback_arg);
	    else {
	      if ((util_is_directory( full_path) && (!util_is_link(full_path)))) 
		util_walk_directory( full_path , file_callback, callback_arg );
	    }

	    free(full_path);
	  }
	}
      } while (dp != NULL);
    }
    closedir( dirH );
  }
}



/**
  This function return true if path corresponds to an existing
  directory, if stat() fails errno is checked for the value ENOENT, in
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



bool util_is_file(const char * path) {
  struct stat stat_buffer;

  if (stat(path , &stat_buffer) == 0)
    return S_ISREG(stat_buffer.st_mode);
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

   Observe the following: 

    * It is easy to be fooled by the optional existence of an extension
      (badly desgined API).

    * The function is **NOT** based purely on string parsing, but also
      on checking stat() output to check if the argument you send in
      is an existing directory (that is done through the
      util_get_path_length() function).


      Ex1: input is an existing directory:
      ------------------------------------
      util_alloc_file_components("/some/existing/path" , &path , &base , &ext)

        path -> "/some/existing/path"
        base -> NULL
        ext  -> NULL
	
	

      Ex2: input is NOT an existing directory:
      ------------------------------------
      util_alloc_file_components("/some/random/not_existing/path" , &path , &base , &ext)
      
      path -> "/some/random/not_existing"
        base -> "path"
        ext  -> NULL
	
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


uid_t util_get_file_uid( const char * file ) {
  struct stat buffer;
  stat( file , &buffer);
  return buffer.st_uid;
}



bool util_file_readable( const char * file ) {
  bool readable;
  FILE * stream = fopen( file , "r");
  if (stream != NULL) {
    readable = true;
    fclose( stream );
  } else 
    readable = false;
  return readable;
}


/**
   Returns the permission mode for the file.
*/

mode_t util_get_file_mode( const char * file ) {
  struct stat buffer;
  stat( file , &buffer );
  return buffer.st_mode & (S_IRWXU + S_IRWXG + S_IRWXO);
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
  const int    min_read      = 256; /* Crirtically small */
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


/** 
    This function will return a pointer to the newest of the two
    files. If one of the files does not exist - the other is
    returned. If none of the files exist - NULL is returned.
*/

char * util_newest_file(const char *file1 , const char *file2) {
  if (util_file_exists(file1)) {
    if (util_file_exists(file2)) {
      /* Actual comparison of two existing files. */
      if (util_file_difftime(file1 , file2) < 0)
	return (char *) file1;
      else
	return (char *) file2;
    } else
      return (char *)file1;   /* Only file1 exists. */
  } else {
    if (util_file_exists(file2))
      return (char *) file2; /* Only file2 exists. */
    else
      return NULL;   /* None of the files exist. */
  }
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


/*
  This function takes a time_t instance as input, and 
  returns the the time broken down in sec:min:hour  mday:month:year.

  The return values are by pointers - you can pass in NULL to any of
  the fields.
*/
void util_set_datetime_values(time_t t , int * sec , int * min , int * hour , int * mday , int * month , int * year) {
  __util_set_timevalues(t , sec , min , hour , mday , month , year);
}


void util_set_date_values(time_t t , int * mday , int * month , int * year) {
  __util_set_timevalues(t , NULL , NULL , NULL , mday , month , year);
}


void util_fprintf_datetime(time_t t , FILE * stream) {
  int sec,min,hour;
  int mday,year,month;
  
  util_set_datetime_values(t , &sec , &min , &hour , &mday , &month , &year);
  fprintf(stream , "%02d/%02d/%4d  %02d:%02d:%02d", mday,month,year,hour,min,sec);
}


void util_fprintf_date(time_t t , FILE * stream) {
  int mday,year,month;
  
  util_set_datetime_values(t , NULL , NULL , NULL , &mday , &month , &year);
  fprintf(stream , "%02d/%02d/%4d", mday,month,year);
}



/* 
   This function takes a pointer to a time_t instance, and shifts the
   value days forward. Observe the calls to localtime_r() which give
   rise to +/- one extra hour of adjustment if we have crossed exactly
   one daylight savings border.

   This code produced erroneus results when compiled with -pg for
   profiling. (Maybe because the ts variable was not properly
   initialized when reading off the first isdst setting??)
*/

void util_inplace_forward_days(time_t * t , double days) {
  struct tm ts;
  int isdst;
  
  localtime_r(t , &ts);
  isdst = ts.tm_isdst;
  (*t) += (int) (days * 3600 * 24);
  localtime_r(t , &ts);
  (*t) += 3600 * (isdst - ts.tm_isdst);  /* Extra adjustment of +/- one hour if we have crossed exactly one daylight savings border. */
}


/**
   This function computes the difference in time between times time1
   and time0: time1 - time0. The return value is the difference in
   seconds (straight difftime output). Observe that the ordering of
   time_t arguments is switched with respect to the difftime
   arguments.
   
   In addition the difference can be broken down in days, hours,
   minutes and seconds if the appropriate pointers are passed in.
*/

   
double util_difftime(time_t start_time , time_t end_time , int * _days , int * _hours , int * _minutes , int *_seconds) {
  int sec_min  = 60;
  int sec_hour = 3600;
  int sec_day  = 24 * 3600;
  double dt = difftime(end_time , start_time);
  double dt0 = dt;
  int days , hours, minutes , seconds;

  days = (int) floor(dt / sec_day );
  dt  -= days * sec_day;
  
  hours = (int) floor(dt / sec_hour);
  dt   -= hours * sec_hour;
  
  minutes = (int) floor(dt / sec_min);
  dt     -= minutes * sec_min;

  seconds = (int) dt;

  if (_seconds != NULL) *_seconds = seconds;
  if (_minutes != NULL) *_minutes = minutes;
  if (_hours   != NULL) *_hours   = hours;
  if (_days    != NULL) *_days    = days;
  
  return dt0;
}



/* Is this dst safe ??? */
double util_difftime_days(time_t start_time , time_t end_time) {
  double dt = difftime(end_time , start_time);
  return dt / (24 * 3600);
}



/*
  Observe that this routine does the following transform before calling mktime:

  1. month -> month - 1;
  2. year  -> year  - 1900;

  Then it is on the format which mktime expects.

*/

time_t util_make_datetime(int sec, int min, int hour , int mday , int month , int year) {
  struct tm ts;
  ts.tm_sec    = sec;
  ts.tm_min    = min;
  ts.tm_hour   = hour;
  ts.tm_mday   = mday;
  ts.tm_mon    = month - 1;
  ts.tm_year   = year  - 1900;
  ts.tm_isdst  = -1;    /* Negative value means mktime tries to determine automagically whether Daylight Saving Time is in effect. */
  {
    time_t t = mktime( &ts );
    if (t == -1) 
      util_abort("%s: failed to make a time_t instance of %02d/%02d/%4d  %02d:%02d:%02d - aborting \n",__func__ , mday,month,year,hour,min,sec);
    
    return t;
  }
}



time_t util_make_date(int mday , int month , int year) {
  return util_make_datetime(0 , 0 , 0 , mday , month , year);
}



static int util_get_month_nr__(const char * _month_name) {
  int month_nr = -1;
  char * month_name = util_alloc_string_copy(_month_name);
  util_strupr(month_name);
  
  if (strncmp(month_name , "JAN" , 3)      == 0) 
    month_nr = 1;
  else if (strncmp(month_name , "FEB" , 3) == 0) 
    month_nr = 2;
  else if (strncmp(month_name , "MAR" , 3) == 0) 
    month_nr = 3;
  else if (strncmp(month_name , "APR" , 3) == 0) 
    month_nr = 4;
  else if (strncmp(month_name , "MAI" , 3) == 0) 
    month_nr = 5;
  else if (strncmp(month_name , "MAY" , 3) == 0) 
    month_nr = 5;
  else if (strncmp(month_name , "JUN" , 3) == 0) 
    month_nr = 6;
  else if (strncmp(month_name , "JUL" , 3) == 0) 
    month_nr = 7;
  else if (strncmp(month_name , "JLY" , 3) == 0)   /* ECLIPSE ambigus on July. */
    month_nr = 7;
  else if (strncmp(month_name , "AUG" , 3) == 0) 
    month_nr = 8;
  else if (strncmp(month_name , "SEP" , 3) == 0) 
    month_nr = 9;
  else if (strncmp(month_name , "OCT" , 3) == 0) 
    month_nr = 10;
  else if (strncmp(month_name , "OKT" , 3) == 0) 
    month_nr = 10;
  else if (strncmp(month_name , "NOV" , 3) == 0) 
    month_nr = 11;
  else if (strncmp(month_name , "DEC" , 3) == 0) 
    month_nr = 12;
  else if (strncmp(month_name , "DES" , 3) == 0) 
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


/**
   This function reallocates the stringlist pointer, making room for
   one more char *, this newly allocated slot is then set to point to
   (a copy of) the new string. The newly reallocated char ** instance
   is the return value from this function.

   Example:
   --------
   char ** stringlist  = (char *[2]) {"One" , "Two"};
   char  * three_string = "Three";
   
   stringlist = util_stringlist_append_copy(stringlist , 2 , three_string);

   This function does allocate memory - but does not have *alloc* in
   the name - hmmmm....??
*/




char ** util_stringlist_append_copy(char ** string_list, int size , const char * append_string) {
  return util_stringlist_append_ref(string_list , size , util_alloc_string_copy(append_string));
}


/**
   This is nearly the same as util_stringlist_append_copy(), but for
   this case only a refernce to the new string is appended.

   Slightly more dangerous to use ...
*/

char ** util_stringlist_append_ref(char ** string_list, int size , const char * append_string) {
  string_list = util_realloc(string_list , (size + 1) * sizeof * string_list , __func__);
  string_list[size] = (char *) append_string;
  return string_list;
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


/**
   This function check that a pointer is different from NULL, and
   frees the memory if that is the case. Observe that the function
   always returns NULL, this means that you can use:

      p = util_safe_free( p );

   Which will both free the memory of p, and set p -> NULL.
*/
 

void * util_safe_free(void *ptr) { 
  if (ptr != NULL) free(ptr); 
  return NULL; 
}




/**
   This function checks whether a string matches a pattern with
   wildcard(s). The pattern can consist of plain string parts (which
   must match verbatim), and an arbitrary number of '*' which will
   match an arbitrary number (including zero) of arbitrary characters.
   
   Examples:
   ---------

   util_string_match("Bjarne" , "Bjarne")    ==> True

   util_string_match("Bjarne" , "jarn")      ==> False   

   util_string_match("Bjarne" , "*jarn*")    ==> True

   util_string_match("Bjarne" , "B*e")       ==> True
   
   util_string_match("Bjarne" , "B*n")       ==> False

   util_string_match("Bjarne" , "*")         ==> True

   util_string_match("Bjarne" , "B***jarne") ==> True

   util_string_match("Bjarne" , "B*r*e")     ==> True

*/



bool util_string_match(const char * string , const char * pattern) {
  const   char    wildcard    = '*';
  const   char   *wildcard_st = "*";

  if (strcmp(wildcard_st , pattern) == 0) 
    return true;
  else {
    bool    match = true;
    char ** sub_pattern;
    int     num_patterns;
    char *  string_ptr;
    util_split_string( pattern , wildcard_st , &num_patterns , &sub_pattern );
    
    if (pattern[0] == '*')
      string_ptr = strstr(string , sub_pattern[0]);
    else
      string_ptr = (strncmp(string , sub_pattern[0] , strlen(sub_pattern[0])) == 0) ? (char * ) string : NULL;
    
    if (string_ptr != NULL) {
      /* Inital part matched */
      string_ptr += strlen( sub_pattern[0] );
      for (int i=1; i < num_patterns; i++) {
	char * match_ptr = strstr(string_ptr , sub_pattern[i]);
	if (match_ptr != NULL) 
	  string_ptr = match_ptr + strlen( sub_pattern[i] );
	else {
	  match = false;
	  break;
	}
      }
      
      /* 
	 We have exhausted the complete pattern - matching all the way.
	 Does it match at the end?
      */
      if (match) {
	if (strlen(string_ptr) > 0) {
	  /* 
	     There is more left at the end of the string; if the pattern
	     ends with '*' that is OK, otherwise the match result is
	     FALSE.
	  */
	  if (pattern[(strlen(pattern) - 1)] != wildcard)
	    match = false;
	}
      }
      
    } else 
      match = false;
    
    util_free_stringlist( sub_pattern , num_patterns);
    return match;
  }
}


void util_free_stringlist(char **list , int N) {
  int i;
  if (list != NULL) {
    for (i=0; i < N; i++) {
      if (list[i] != NULL)
	free(list[i]);
    }
    free(list);
  }
}


char ** util_alloc_stringlist(int N, int len) {
  int i;
  char **list = calloc(N , sizeof *list);
  for (i=0; i < N; i++)
    list[i] = util_malloc(len , __func__);
  return list;
}



/**
   This function will reallocate the string s1 to become the sum of s1
   and s2.
*/

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
	fileList[filenr].filename   = util_alloc_filename(enspath , entry->d_name , NULL);
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


/**
   sep_set = string with various characters, i.e. " \t" to split on.
*/

void util_split_string(const char *line , const char *sep_set, int *_tokens, char ***_token_list) {
  int offset;
  int tokens , token , token_length;
  char **token_list;

  offset = strspn(line , sep_set); 
  tokens = 0;
  do {
    /*
      if (line[offset] == '\"') {
      seek for terminating ".
      }
    */
    token_length = strcspn(&line[offset] , sep_set);
    if (token_length > 0)
      tokens++;
    
    offset += token_length;
    offset += strspn(&line[offset] , sep_set);
  } while (line[offset] != '\0');

  if (tokens > 0) {
    token_list = util_malloc(tokens * sizeof * token_list , __func__);
    offset = strspn(line , sep_set);
    token  = 0;
    do {
      token_length = strcspn(&line[offset] , sep_set);
      if (token_length > 0) {
	token_list[token] = util_alloc_substring_copy(&line[offset] , token_length);
	token++;
      } else
	token_list[token] = NULL;
      
      offset += token_length;
      offset += strspn(&line[offset] , sep_set);
    } while (line[offset] != '\0');
  } else
    token_list = NULL;
  
  *_tokens     = tokens;
  *_token_list = token_list;
}


/**
   This function will split the input string in two parts, it will
   split on occurence of one or several of the characters in
   sep_set. 


   o If split_on_first is true it will split on the first occurence of
     split_set, and otherwise it will split on the last:

       util_binary_split_string("A:B:C:D , ":" , true  , ) => "A"     & "B:C:D"
       util_binary_split_string("A:B:C:D , ":" , false , ) => "A:B:C" & "D"


   o Characters in the split_set at the front (or back if
     split_on_first == false) are discarded _before_ the actual
     splitting process.

       util_binary_split_string(":::A:B:C:D" , ":" , true , )  => "A" & "B:C:D"


   o If no split is found the whole content is in first_part, and
     second_part is NULL. If the input string == NULL, both return
     strings will be NULL.

*/


void util_binary_split_string(const char * __src , const char * sep_set, bool split_on_first , char ** __first_part , char ** __second_part) {
  char * first_part = NULL;
  char * second_part = NULL;
  if (__src != NULL) {
    char * src;
    int pos;
    if (split_on_first) {
      /* Removing leading separators. */
      pos = 0;
      while ((pos < strlen(__src)) && (strchr(sep_set , __src[pos]) != NULL))
	pos += 1;
      if (pos == strlen(__src))  /* The string consisted ONLY of separators. */
	src = NULL;
      else
	src = util_alloc_string_copy(&__src[pos]);
    } else {
      /*Remove trailing separators. */
      pos = strlen(__src) - 1;
      while ((pos >= 0) && (strchr(sep_set , __src[pos]) != NULL))
	pos -= 1;
      if (pos < 0)
	src = NULL;
      else
	src = util_alloc_substring_copy(__src , pos + 1);
    }

    
    /* 
       OK - we have removed all leading (or trailing) separators, and we have
       a valid string which we can continue with.
    */
    if (src != NULL) {
      int pos;
      int start_pos , delta , end_pos;
      if (split_on_first) {
	start_pos = 0;
	delta     = 1;
	end_pos   = strlen(src);
      } else {
	start_pos = strlen(src) - 1;
	delta     = -1;
	end_pos   = -1;
      }

      pos = start_pos;
      while ((pos != end_pos) && (strchr(sep_set , src[pos]) == NULL)) 
	pos += delta;
      /* 
	 OK - now we have either iterated through the whole string - or
	 we hav found a character in the sep_set. 
      */
      if (pos == end_pos) {
	/* There was no split. */
	first_part = util_alloc_string_copy( src );
	second_part   = NULL;
      } else {
	int sep_start = 0;
	int sep_end   = 0;
	if (split_on_first)
	  sep_start = pos;
	else
	  sep_end = pos;
	/* Iterate through the separation string - can be e.g. many " " */
	while ((pos != end_pos) && (strchr(sep_set , src[pos]) != NULL))
	  pos += delta;

	if (split_on_first) {
	  sep_end = pos;
	  first_part = util_alloc_substring_copy(src , sep_start);
	  
	  if (sep_end == end_pos)
	    second_part = NULL;
	  else
	    second_part = util_alloc_string_copy( &src[sep_end] );
	} else {
	  sep_start = pos;
	  if (sep_start == end_pos) {
	    // ":String" => (NULL , "String")
	    first_part = NULL;
	    second_part = util_alloc_string_copy( &src[sep_end+1] );
	  } else {
	    first_part  = util_alloc_substring_copy( src , sep_start + 1);
	    second_part = util_alloc_string_copy( &src[sep_end + 1]);
	  }
	}
      }
      free(src);
    }
  }
  *__first_part  = first_part;
  *__second_part = second_part;
}


/**
   The return value is the number of substitutions which have been
   performed.

   Observe that the variable _buffer_size should point to an integer
   which contains the size of the buffer; this will be updated during
   run. Observe that *_buffer_size should be the TOTAL size of the
   buffer, i.e. if if the buffer is terminated with '\0' that should
   be included in the buffer_size.
*/
  
int static util_string_replace_inplace__(char ** _buffer , int *_buffer_size , const char * expr , const char * subs) {
  char * buffer      	   = *_buffer;
  int    buffer_size 	   = *_buffer_size;
  int len_expr  	   = strlen(expr);
  int len_subs  	   = strlen(subs);
  int    size   	   = strlen(buffer);
  int    offset 	   = 0;     
  int    match_count       = 0;

  char  * match = NULL;
  do {
    match = strstr(&buffer[offset] ,  expr);
    
    if (match != NULL) {
      /* 
	 Can not use pointer arithmetic here - because the underlying
	 buffer pointer might be realloced.
      */
      int    start_offset  = match            - buffer;
      int    end_offset    = match + len_expr - buffer;
      int    target_offset = match + len_subs - buffer;
      int    new_size      = size  + len_subs - len_expr;
      if (new_size >= (buffer_size - 1)) {
	buffer_size += buffer_size + 2*len_subs;
	buffer = util_realloc( buffer , buffer_size , __func__);
      }
      {
	char * target    = &buffer[target_offset];
	char * match_end = &buffer[end_offset];
	memmove(target , match_end , 1 + size - end_offset);
      }
      
      memcpy(&buffer[start_offset] , subs , len_subs);
      offset = start_offset + len_subs;
      size   = new_size;
      match_count++;
    }
  } while (match != NULL && offset < strlen(buffer));
    
    
  *_buffer      = buffer;
  *_buffer_size = buffer_size;
  return match_count;
}



int util_string_replace_inplace(char ** _buffer , int *_buffer_size , const char * expr , const char * subs) {
  return util_string_replace_inplace__(_buffer , _buffer_size , expr , subs);
}




/**
  This allocates a copy of buff_org where occurences of the string expr are replaced with subs.
*/
char * util_string_replace_alloc(const char * buff_org, const char * expr, const char * subs)
{
  int buffer_size   = strlen(buff_org) * 2;
  char * new_buffer = util_malloc(buffer_size * sizeof * new_buffer , __func__);
  memcpy(new_buffer , buff_org , strlen(buff_org) + 1);
  util_string_replace_inplace__( &new_buffer , &buffer_size , expr , subs);
  
  {
    int size = strlen(new_buffer);
    new_buffer = util_realloc(new_buffer, (size + 1) * sizeof * new_buffer, __func__);
  }

  return new_buffer;
}



/**
  This allocates a copy of buff_org where occurences of expr[i] are replaced with subs[i] for i=1,..,num_expr.
*/
char * util_string_replacen_alloc(const char * buff_org, int num_expr, const char ** expr, const char ** subs)
{
  int buffer_size   = strlen(buff_org) * 2;
  char * new_buffer = util_malloc(buffer_size * sizeof * new_buffer , __func__);
  memcpy(new_buffer , buff_org , strlen(buff_org) + 1);

  for(int i=0; i<num_expr; i++)
    util_string_replace_inplace__( &new_buffer , &buffer_size , expr[i] , subs[i]);
  
  int size = strlen(new_buffer);
  new_buffer = util_realloc(new_buffer, (size + 1) * sizeof * new_buffer, __func__);

  return new_buffer;
}


/**
  This will alloc a copy of buff_org were char's in the last strings are removed.
*/
char * util_string_strip_chars_alloc(const char * buff_org, const char * chars)
{
  int len_org = strlen(buff_org);
  int pos_org = 0;
  int pos_new = 0;

  char * buff_new = util_malloc( (len_org +1) * sizeof * buff_new, __func__);

  while(pos_org < len_org)
  {
    int pos_off = strcspn(buff_org + pos_org, chars);
    if(pos_off > 0)
    {
      memmove(buff_new + pos_new, buff_org + pos_org, pos_off * sizeof * buff_new);  
      pos_org += pos_off;
      pos_new += pos_off;
    }

    pos_off = strspn(buff_org + pos_org, chars);
    if(pos_off > 0)
    {
      pos_org += pos_off;
    }
  }
  buff_new[pos_new + 1] = '\0';
  buff_new = util_realloc(buff_new, (pos_new + 1) * sizeof buff_new, __func__);

  return buff_new;
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

/*  
   The util_fwrite_string / util_fread_string are BROKEN when it comes
   to NULL / versus an empty string "":

    1. Writing a "" string what is actually written to disk is: "0\0",
       whereas the disk content when writing NULL is "0".

    2. When reading back we find the '0' - but it is impossible to
       determine whether we should interpret this as a NUL or as "".

   When the harm was done, with files allover the place, it is "solved"
   as follows:

    1. Nothing is changed when writing NULL => '0' to disk.
    
    2. When writing "" => '0-1\0' to disk. The -1 is the magic length
       signifying that the following string is "".
*/



void util_fwrite_string(const char * s, FILE *stream) {
  int len = 0;
  if (s != NULL) {
    len = strlen(s);
    if (len == 0) 
      util_fwrite_int(-1 , stream);  /* Writing magic string for "" */
    else
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
  } else if (len == -1) /* Magic length for "" */ {
    s = util_malloc(1 , __func__);
    util_fread(s , 1 , 1 , stream , __func__);
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
  } else if (len == -1) /* Magic length for "" */ {
    s = util_realloc(s , 1 , __func__);
    util_fread(s , 1 , 1 , stream , __func__);
  } 
  return s;
}


void util_fskip_string(FILE *stream) {
  int len;
  util_fread(&len , sizeof len , 1 , stream , __func__);
  if (len == -1) /* Magig string for "" - skip the '\0' */
    len = 0;
  fseek(stream , len + 1 , SEEK_CUR);
}



void util_fwrite_bool  (bool value , FILE * stream)   { UTIL_FWRITE_SCALAR(value , stream); }
void util_fwrite_int   (int value , FILE * stream)    { UTIL_FWRITE_SCALAR(value , stream); }
void util_fwrite_long  (long value , FILE * stream)    { UTIL_FWRITE_SCALAR(value , stream); }
void util_fwrite_double(double value , FILE * stream) { UTIL_FWRITE_SCALAR(value , stream); }

void util_fwrite_int_vector   (const int * value    , int size , FILE * stream, const char * caller) { util_fwrite(value , sizeof * value, size , stream, caller); }
void util_fwrite_double_vector(const double * value , int size , FILE * stream, const char * caller) { util_fwrite(value , sizeof * value, size , stream, caller); }
void util_fwrite_char_vector  (const char * value   , int size , FILE * stream, const char * caller) { util_fwrite(value , sizeof * value, size , stream, caller); }

void util_fread_char_vector(char * ptr , int size , FILE * stream , const char * caller) {
  util_fread(ptr , sizeof * ptr , size , stream , caller);
}



int util_fread_int(FILE * stream) {
  int file_value;
  UTIL_FREAD_SCALAR(file_value , stream);
  return file_value;
}

long util_fread_long(FILE * stream) {
  long file_value;
  UTIL_FREAD_SCALAR(file_value , stream);
  return file_value;
}


bool util_fread_bool(FILE * stream) {
  bool file_value;
  UTIL_FREAD_SCALAR(file_value , stream);
  return file_value;
}


void util_fskip_int(FILE * stream) {
  fseek( stream , sizeof (int) , SEEK_CUR);
}

void util_fskip_long(FILE * stream) {
  fseek( stream , sizeof (long) , SEEK_CUR);
}

void util_fskip_bool(FILE * stream) {
  fseek( stream , sizeof (bool) , SEEK_CUR);
}


/*****************************************************************/

size_t util_size_t_min(size_t a , size_t b) {
  return (a < b) ? a : b;
}

size_t util_size_t_max(size_t a , size_t b) {
  return (a > b) ? a : b;
}

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



/**
   Scans through a vector of doubles, and finds min and max
   values. They are returned by reference.
*/
  
void util_double_vector_max_min(int N , const double *vector, double *_max , double *_min) {
  double min =  1e100; /* How should this be done ??? */
  double max = -1e100;
  int i;
  for (i = 0; i < N; i++) {
    if (vector[i] > max)
      max = vector[i];

    /* Can not have else here - because same item might succed on both tests. */
    
    if (vector[i] < min)
      min = vector[i];
  }
  *_max = max;
  *_min = min;
}



double util_double_vector_mean(int N, const double * vector) {
  double mean = 0.0;
  
  for(int i=0; i<N; i++)
    mean = mean + vector[i];

  return mean / N;
}



double util_double_vector_stddev(int N, const double * vector) {
  if(N <= 1)
    return 0.0;
  
  double   stddev         = 0.0;
  double   mean           = util_double_vector_mean(N, vector);
  double * vector_shifted = util_malloc(N * sizeof *vector_shifted, __func__);

  for(int i=0; i<N; i++)
    vector_shifted[i] = vector[i] - mean;

  for(int i=0; i<N; i++)
    stddev = stddev + vector_shifted[i] * vector_shifted[i];

  free(vector_shifted);

  return sqrt( stddev / (N-1));
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


   
/**
   This function will update *value so that on return ALL bits which
   are set in bitmask, are also set in value. No other bits in *value
   should be modified - i.e. it is a logical or.
*/

void util_bitmask_on(int * value , int mask) {
  int tmp = *value;
  tmp = (tmp | mask);
  *value = tmp;
}


/* 
   Opens a file, and locks it for exclusive acces. fclose() will
   release all locks.
*/

FILE * util_fopen_lockf(const char * filename, const char * mode) {
  int flags = 0; /* Compiler shut up */
  int fd;
  int lock_status;

  flags = O_RDWR;  /* Observe that the open call must have write option to be able to place a lock - even though we only attempt to read from the file. */
  if (strcmp(mode , "w") == 0)
    flags += O_CREAT;
  
  fd = open(filename , flags);
  if (fd == -1) 
    util_abort("%s: failed to open:%s with flags:%d \n",__func__ , filename , flags);
  
  lock_status = lockf(fd , F_LOCK , 0);
  if (lock_status != 0)
    util_abort("%s: failed to lock file: %s %s(%d) \n",__func__ , filename , strerror(errno) , errno);
  
  return fdopen(fd , mode);
}


static pthread_mutex_t __fwrite_block_mutex = PTHREAD_MUTEX_INITIALIZER; /* Used to ensure that only one thread displays the block message. */
static void __block_full_disk(const char * filename) {                   /* Filename can be NULL */
  if (pthread_mutex_trylock( &__fwrite_block_mutex ) == 0) {
    /** 
        Okay this was the first thread to register a full disk. This
        thread will display the message and await user input. If other
        threads also encounter full disk they will just spinn sleeping
        until this thread terminates.
    */
    fprintf(stderr,"********************************************************************\n");
    fprintf(stderr,"** The filesystem seems to be full  - and the program is veeeery  **\n");
    fprintf(stderr,"** close to going down in flames. You can try clearing space, and **\n");
    fprintf(stderr,"** then press return [with fingers crossed :-)].                  **\n");
    fprintf(stderr,"********************************************************************\n");
    getc( stdin ); /* Block while user clears some disk space. */
  } else {
    /* 
       Another thread is already blocking - waiting for user
       input. This thread will block, waiting on the first thread.
    */
    while (true) {
      usleep( 500000 ); /* half a second */
            
      if (pthread_mutex_trylock( &__fwrite_block_mutex ) == 0) {
        usleep( 5000 );  /* 5 ms - to avoid a herd of threads hitting the filesystem concurrently.*/
        break;           /* The user has entered return - and the main blocking thread has been released. */
      }
    }
  }
  pthread_mutex_unlock( &__fwrite_block_mutex );
}



FILE * util_fopen(const char * filename , const char * mode) {
  FILE * stream = fopen(filename , mode);
  if (stream == NULL) {
    /* 
       We try to handle "No space left on the device" by letting the user 
       get a chance to clean out the disk.
    */
    if (errno == ENOSPC) {
      __block_full_disk(filename);
      return util_fopen(filename , mode); /* Try again. */
    } else
      util_abort("%s: failed to open:%s with mode:\'%s\' - error:%s(%d) \n",__func__ , filename , mode , strerror(errno) , errno);
  }
  return stream;
}




void util_fwrite(const void *ptr , size_t element_size , size_t items, FILE * stream , const char * caller) {
  int items_written = fwrite(ptr , element_size , items , stream);
  if (items_written != items) {
    /* 
       We try to handle "No space left on the device" by letting the user 
       get a chance to clean out the disk.
    */
    if (errno == ENOSPC) {
      __block_full_disk( NULL );
      const char * char_ptr = ptr;
      util_fwrite( &char_ptr[ items_written * element_size ] , element_size , items - items_written , stream , caller);
    } else
      util_abort("%s/%s: only wrote %d/%d items to disk - aborting: %s(%d) .\n",caller , __func__ , items_written , items , strerror(errno) , errno);
  }
}


void util_fread(void *ptr , size_t element_size , size_t items, FILE * stream , const char * caller) {
  int items_read = fread(ptr , element_size , items , stream);
  if (items_read != items) 
    util_abort("%s/%s: only read %d/%d items from disk - aborting.\n %s(%d) \n",caller , __func__ , items_read , items , strerror(errno) , errno);
}



void util_fread_from_buffer(void * ptr , size_t element_size , size_t items , char ** buffer) {
  int bytes = element_size * items;
  memcpy( ptr , *buffer , bytes);
  *buffer += bytes;
}



#undef ABORT_READ
#undef ABORT_WRITE


/*****************************************************************/

void * util_realloc(void * old_ptr , size_t new_size , const char * caller) {
  /* The realloc documentation as ambigous regarding realloc() with size 0 - WE return NULL. */
  if (new_size == 0) { 
    if (old_ptr != NULL)
      free(old_ptr);
    return NULL;
  } else {
    void * tmp = realloc(old_ptr , new_size);
    if (tmp == NULL)
      util_abort("%s: failed to realloc %d bytes - aborting \n",caller , new_size);
    return tmp;
  }
}


/**
   This function is a super-thin wrapper around malloc. It allocates
   the number of bytes you ask for. If the return value from malloc()
   is NULL the routine will abort().

   If you are actually interested in handling malloc() failures in a
   decent way, you should not use this routine.
*/


void * util_malloc(size_t size , const char * caller) {
  void * data;
  if (size == 0) 
    /* Not entirely clear from documentation what you get when you call malloc( 0 ); */
    data = NULL;
  else {
    data = malloc(size);
    if (data == NULL) 
      util_abort("%s: failed to allocate %d bytes - aborting \n",caller , size);

    /* 
       Initializing with something different from zero - hopefully
       errors will pop up more easily this way?
    */
    memset(data , 255 , size);
  }
  return data;
}


/**
   Allocates byte_size bytes of storage, and initializes content with
   the value found in src.
*/

void * util_alloc_copy(const void * src , size_t byte_size , const char * caller) {
  if (byte_size == 0 && src == NULL)
    return NULL;
  {
    void * new = util_malloc(byte_size , caller);
    memcpy(new , src , byte_size);
    return new;
  }
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
  compressed copy into qqthe target buffer zbuffer. On input data_size
  should be the *number of bytes* in data. compressed_size should be a
  reference to the size (in bytes) of zbuffer, on return this has been
  updated to reflect the new compressed size.
*/

void util_compress_buffer(const void * data , int data_size , void * zbuffer , unsigned long * compressed_size) {
  int compress_result;
  if (data_size > 0) {
    compress_result = compress(zbuffer , compressed_size , data , data_size);
    if (compress_result != Z_OK) 
      util_abort("%s: returned %d - different from Z_OK - aborting\n",__func__ , compress_result);
  } else
    *compressed_size = 0;
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


/**
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

Observe that the functions util_fwrite_compressed() and
util_fread_compressed must be used as a pair, the files can **N O T**
be interchanged with normal calls to gzip/gunzip. To avoid confusion
it is therefor strongly advised NOT to give the files a .gz extension.

*/

void util_fwrite_compressed(const void * _data , int size , FILE * stream) {
  if (size == 0) {
    fwrite(&size , sizeof size , 1 , stream);
    return;
  }
  {
    const char * data = (const char *) _data;
    const int max_buffer_size      = 128 * 1048580; /* 128 MB */
    int       required_buffer_size = (int) ceil(size * 1.001 + 64);
    int       buffer_size , block_size;
    void *    zbuffer;
    
    buffer_size = util_int_min(required_buffer_size , max_buffer_size);
    do {
      zbuffer = malloc(buffer_size);
      if (zbuffer == NULL)
	buffer_size /= 2;
    } while(zbuffer == NULL);
    memset(zbuffer , 0 , buffer_size);
    block_size = (int) (floor(buffer_size / 1.002) - 64);
    
    {
      int header_write;
      header_write  = fwrite(&size        , sizeof size        , 1 , stream);
      header_write += fwrite(&buffer_size , sizeof buffer_size , 1 , stream);
      if (header_write != 2)
	util_abort("%s: failed to write header to disk: %s \n",__func__ , strerror(errno));
    }
    
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

/**
  This function is used to read compressed data from file, observe
  that the file must have been created with util_fwrite_compressed()
  first. Trying to read a file compressed with gzip will fail.
*/

void util_fread_compressed(void *__data , FILE * stream) {
  char * data = (char *) __data;
  int buffer_size;
  int size , offset;
  void * zbuffer;
  
  fread(&size  , sizeof size , 1 , stream); 
  if (size == 0) return;


  fread(&buffer_size , sizeof buffer_size , 1 , stream);
  zbuffer = util_malloc(buffer_size , __func__);
  offset = 0;
  do {
    unsigned long compressed_size;
    unsigned long block_size = size - offset;
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



/**
   Allocates storage and reads in from compressed data from disk. If the
   data on disk have zero size, NULL is returned.
*/

void * util_fread_alloc_compressed(FILE * stream) {
  long   current_pos = ftell(stream);
  char * data;
  int    size;

  fread(&size  , sizeof size , 1 , stream); 
  if (size == 0) 
    return NULL;
  else {
    fseek(stream , current_pos , SEEK_SET);
    data = util_malloc(size , __func__);
    util_fread_compressed(data , stream);
    return data;
  }
}


/**
   Returns the **UNCOMPRESSED** size of a compressed section. 
*/

int util_fread_sizeof_compressed(FILE * stream) {
  long   pos = ftell(stream);
  int    size;

  fread(&size  , sizeof size , 1 , stream); 
  fseek(  stream , pos , SEEK_SET );
  return size;
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
   These small functions write formatted values onto a stream. The
   main point about these functions is to avoid creating small one-off
   format strings. The character base_fmt should be 'f' or 'g'
*/

void util_fprintf_double(double value , int width , int decimals , char base_fmt , FILE * stream) {
  char * fmt = util_alloc_sprintf("%c%d.%d%c" , '%' , width , decimals , base_fmt);
  fprintf(stream , fmt , value);
  free(fmt);
}


void util_fprintf_int(int value , int width , FILE * stream) {
  char fmt[32];
  sprintf(fmt , "%%%dd" , width);
  fprintf(stream , fmt , value);
}



void util_fprintf_string(const char * s , int width , string_alignement_type alignement , FILE * stream) {
  char fmt[32];
  int i;
  if (alignement == left_pad) {
    i = 0;
    if (width > strlen(s)) {
      for (i=0; i < (width - strlen(s)); i++) 
	fputc(' ' , stream);
    }
    fprintf(stream , s);
  } else if (alignement == right_pad) {
    sprintf(fmt , "%%-%ds" , width);
    fprintf(stream , fmt , s);
  } else {
    int total_pad  = width - strlen(s);
    int front_pad  = total_pad / 2;
    int back_pad   = total_pad - front_pad;
    int i;
    util_fprintf_string(s , front_pad + strlen(s) , left_pad , stream);
    for (i=0; i < back_pad; i++)
      fputc(' ' , stream);
  }
}






/**
   This function allocates a string acoording to the fmt
   specification, and arguments. The arguments (except the format) are
   entered as a variable length argument list, and the function is
   basically a thin wrapper around vsnprintf().
   
   Example of usage:
   
   char * s = util_alloc_sprintf("/%s/File:%04d/%s" , "prefix" , 67 , "Suffix");
   
   => s = /prefix/File:0067/Suffix
   
   Observe that when it is based in vsnprintf() essentially no
   error-checking is performed.
*/


char * util_alloc_sprintf(const char * fmt , ...) {
  char *s = NULL;
  va_list ap;
  va_start(ap , fmt);
  {
    int length;
    va_list tmp_va;
    va_copy(tmp_va , ap);
    length = vsnprintf(s , 0 , fmt , tmp_va);
    s = util_malloc(length + 1 , __func__);
  }
  vsprintf(s , fmt , ap);
  va_end(ap);
  return s;
}



char * util_realloc_sprintf(char * s , const char * fmt , ...) {
  va_list ap;
  va_start(ap , fmt);
  {
    int length;
    va_list tmp_va;
    va_copy(tmp_va , ap);
    length = vsnprintf(s , 0 , fmt , tmp_va);
    s = util_realloc(s , length + 1 , __func__);
  }
  vsprintf(s , fmt , ap);
  va_end(ap);
  return s;
}



/**
   This function searches through the content of the (currently set)
   PATH variable, and allocates a string containing the full path
   (first match) to the executable given as input. 

   * If the entered executable already is an absolute path, a copy of
     the input is returned *WITHOUT* consulting the PATH variable (or
     checking that it exists).

   * If the executable starts with "./" getenv("PWD") is prepended. 

   * If the executable is not found in the PATH list NULL is returned.
*/
   

char * util_alloc_PATH_executable(const char * executable) {
  if (util_is_abs_path(executable)) {
    if (util_is_executable(executable))
      return util_alloc_string_copy(executable);
    else
      return NULL;
  } else if (strncmp(executable , "./" , 2) == 0) {
    char * path = util_alloc_filename(getenv("PWD") , &executable[2] , NULL);
    /* The program has been invoked as ./xxxx */
    if (util_is_file(path) && util_is_executable( path )) 
      return path; 
    else {
      free( path );
      return NULL;
    }
  } else {
    char *  full_path = NULL;
    char *  path_env  = getenv("PATH");
    if (path_env != NULL) {
      bool    cont = true;
      char ** path_list;
      int     path_size , ipath;
      
      ipath = 0;
      util_split_string(getenv("PATH") , ":" , &path_size , &path_list);
      while ( cont ) {
	char * current_attempt = util_alloc_filename(path_list[ipath] , executable , NULL);
	if ( util_is_file( current_attempt ) && util_is_executable( current_attempt )) {
	  full_path = current_attempt;
	  cont = false;
	} else {
	  free(current_attempt);
	  ipath++;
	  if (ipath == path_size)
	    cont = false;
	}
      }
      util_free_stringlist(path_list , path_size);
    }
    return full_path;
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
  char *tmp_file = util_alloc_tmp_file("/tmp" , "addr2line" , true);
  char * adress;
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
  
  {
    char ** argv;
    
    argv    = util_malloc(3 * sizeof * argv , __func__);
    argv[0] = util_alloc_string_copy("--functions");
    argv[1] = util_alloc_sprintf("--exe=%s" , executable);
    argv[2] = util_alloc_string_copy(adress);
    
    util_vfork_exec("addr2line" , 3  , (const char **) argv , true , NULL , NULL , NULL , tmp_file , NULL);
    util_free_stringlist(argv , 3);
  }
  
  {
    bool at_eof;
    FILE * stream = util_fopen(tmp_file , "r");
    *func_name = util_fscanf_alloc_line(stream , &at_eof);
    *file_line = util_fscanf_alloc_line(stream , &at_eof);
    fclose(stream);
  }
  util_unlink_existing(tmp_file);
  free(adress);
  free(tmp_file);
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
    char * full_path = util_alloc_PATH_executable( path );
    free(path);
    return full_path;
  }
}


/**
  This function prints a message to stderr and aborts. The function is
  implemented with the help of a variable length argument list - just
  like printf(fmt , arg1, arg2 , arg3 ...);

  Observe that it is __VERY__ important that the arguments and the
  format string match up, otherwise the util_abort() routine will hang
  indefinetely.

  A backtrace is also included, with the help of the exernal utility
  addr2line, this backtrace is converted into usable
  function/file/line information (provided the required debugging
  information is compiled in).
*/


static pthread_mutex_t __abort_mutex  = PTHREAD_MUTEX_INITIALIZER; /* Used purely to serialize the util_abort() routine. */
static char * __abort_program_message = NULL;                      /* Can use util_abort_append_version_info() to fill this with version info+++ wich will be printed when util_abort() is called. */

void util_abort_append_version_info(const char * msg) {
  __abort_program_message = util_strcat_realloc( __abort_program_message , msg );
}

void util_abort_free_version_info() {
  __abort_program_message = util_safe_free( __abort_program_message );
}



void util_abort(const char * fmt , ...) {
  pthread_mutex_lock( &__abort_mutex ); /* Abort before unlock() */
  {
    const bool include_backtrace = true;
    va_list ap;
    va_start(ap , fmt);
    printf("\n\n");
    fprintf(stderr,"\n\n");
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
  
      if (__abort_program_message != NULL) {
	fprintf(stderr,"--------------------------------------------------------------------------------\n");
	fprintf(stderr,"%s",__abort_program_message);
	fprintf(stderr,"--------------------------------------------------------------------------------\n");
      }

      fprintf(stderr,"\n");
      fprintf(stderr,"****************************************************************************\n");
      fprintf(stderr,"**                                                                        **\n");
      fprintf(stderr,"**           A fatal error occured, and we have to abort.                 **\n");
      fprintf(stderr,"**                                                                        **\n");
      fprintf(stderr,"**  We now *try* to provide a backtrace, which would be very useful       **\n");
      fprintf(stderr,"**  when debugging. The process of making a (human readable) backtrace    **\n");
      fprintf(stderr,"**  is quite complex, among other things it involves several calls to the **\n");
      fprintf(stderr,"**  external program addr2line. We have arrived here because the program  **\n");
      fprintf(stderr,"**  state is already quite broken, so the backtrace might be (seriously)  **\n");
      fprintf(stderr,"**  broken as well.                                                       **\n");
      fprintf(stderr,"**                                                                        **\n");
      fprintf(stderr,"****************************************************************************\n");
      size       = backtrace(array , max_bt);
      strings    = backtrace_symbols(array , size);    
      executable = util_bt_alloc_current_executable(strings[0]);
      if (executable != NULL) {
	fprintf(stderr,"Current executable : %s \n",executable);
	
	func_list      = util_malloc(size * sizeof * func_list      , __func__);
	file_line_list = util_malloc(size * sizeof * file_line_list , __func__);
	
	for (i=0; i < size; i++) {
	  util_addr2line_lookup(executable , strings[i] , &func_list[i] , &file_line_list[i]);
	  max_func_length = util_int_max(max_func_length , strlen(func_list[i]));
	}
	
	{
	  char string_fmt[64];
	  sprintf(string_fmt, " #%s02d %s-%ds(..) in %ss   \n" , "%" , "%" , max_func_length , "%");
	  fprintf(stderr , "--------------------------------------------------------------------------------\n");
	  for (i=0; i < size; i++) {
	    
	    int line_nr;
	    if (util_sscanf_int(file_line_list[i] , &line_nr))
	      fprintf(stderr, string_fmt , i , func_list[i], file_line_list[i]);
	    else
	      fprintf(stderr, string_fmt , i , func_list[i], file_line_list[i]);
	  }
	  fprintf(stderr , "--------------------------------------------------------------------------------\n");
	  util_free_stringlist(func_list      , size);
	  util_free_stringlist(file_line_list , size);
	}
      } else
	fprintf(stderr,"Could not determine executable file for:%s - no backtrace. \n",strings[0]);
      
      free(strings);
      free(executable);
    }
    abort();
  }
  pthread_mutex_unlock( &__abort_mutex );
}



/**
  This function is intended to be installed as a signal
  handler, so we can get a traceback from signals like SIGSEGV.
  
  To install the signal handler:

  #include <signal.h>
  ....
  ....
  signal(SIGSEGV , util_abort_signal);


  The various signals can be found in: /usr/include/bits/signum.h
*/
  

void util_abort_signal(int signal) {
  util_abort("Program recieved signal:%d\n" , signal);
}


void util_exit(const char * fmt , ...) {
  va_list ap;
  va_start(ap , fmt);
  vfprintf(stderr , fmt , ap);
  exit(1);
}
    
  



/** 
    This funny function is used to block execution while a file is
    growing. It is a completely heuristic attempt to ensure that the
    writing of a certain file is finished before execution continues.

    It is assumed (and not checked for) that the file already exists.
*/

void util_block_growing_file(const char * file) {
  const int usleep_time = 10000; /* 1/100 of a second */
  int prev_size;
  int current_size = 0;
  do {
    prev_size = current_size;
    usleep(usleep_time);
    current_size = util_file_size(file);
  } while (current_size != prev_size);
}


/**
   This is much similar to the previous function, this function blocks
   until the number of entries in directory is constant (it does not
   consider wether the size of the files is changing), again this is
   quite special-purpose function for the enkf + ECLIPSE cooperation.
*/ 

void util_block_growing_directory(const char * directory) {
  const int usleep_time = 10000; /* 1/100 of a second */
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


/** 
    A small function used to redirect a file descriptior,
    only used as a helper utility for util_vfork_exec().
*/
    
static void __util_redirect(int src_fd , const char * target_file , int open_flags) {
  int new_fd = open(target_file , open_flags , 0644);
  dup2(new_fd , src_fd);
  close(new_fd);
}




/**
   This function does the following:

    1. Fork current process.
    2. if (run_path != NULL) chdir(run_path)
    3. The child execs() to run executable.
    4. Parent can wait (blocking = true) on the child to complete executable.

   If the executable is an absolute path it will run the command with
   execv(), otherwise it will use execvp() which will (try) to look up
   the executable with the PATH variable.

   argc / argv are the number of arguments and their value to the
   external executable. Observe that prior to calling execv the argv
   list is prepended with the name of the executable (convention), and
   a NULL pointer is appended (requirement by execv).

   If stdout_file != NULL stdout is redirected to this file.  Same
   with stdin_file and stderr_file.

   If target_file != NULL, the parent will check that the target_file
   has been created before returning; and abort if not. In this case
   you *MUST* have blocking == true, otherwise it will abort on
   internal error.


   The return value from the function is the pid of the child process;
   this is (obviously ?) only interesting if the blocking argument is
   'false'.

   Example:
   --------
   util_fork_exec("/local/gnu/bin/ls" , 1 , (const char *[1]) {"-l"} ,
   true , NULL , NULL , NULL , "listing" , NULL);

   
   This program will run the command 'ls', with the argument '-l'. The
   main process will block, i.e. wait until the 'ls' process is
   complete, and the results of the 'ls' operation will be stored in
   the file "listing". If the 'ls' should want to print something on
   stderr, it will go there, as stderr is not redirected.

*/

pid_t util_vfork_exec(const char * executable , int argc , const char ** argv , 
		      bool blocking , const char * target_file , const char  * run_path , 
		      const char * stdin_file , const char * stdout_file , const char * stderr_file) {
  const char  ** __argv = NULL;
  pid_t child_pid;
  
  if (target_file != NULL && blocking == false) 
    util_abort("%s: When giving a target_file != NULL - you must use the blocking semantics. \n",__func__);

  child_pid = vfork();
  if (child_pid == -1) {
    fprintf(stderr,"Error: %s(%d) \n",strerror(errno) , errno);
    util_abort("%s: fork() failed when trying to run external command:%s \n",__func__ , executable);
  }

  if (child_pid == 0) {
    /* This is the child */
    int iarg;

    nice(19);    /* Remote process is run with nice(19). */
    if (run_path != NULL) {
      if (chdir(run_path) != 0) 
	util_abort("%s: failed to change to directory:%s  %s \n",__func__ , run_path , strerror(errno));
    }

    if (stdout_file != NULL) {
      /** This is just to invoke the "block on full disk behaviour" before the external program starts. */
      FILE * stream = util_fopen( stdout_file , "w");
      fclose(stream);
      __util_redirect(1 , stdout_file , O_WRONLY | O_TRUNC | O_CREAT);
    }
    if (stderr_file != NULL) {
      /** This is just to invoke the "block on full disk behaviour" before the external program starts. */
      FILE * stream = util_fopen( stderr_file , "w");
      fclose(stream);
      __util_redirect(2 , stderr_file , O_WRONLY | O_TRUNC | O_CREAT);
    }
    if (stdin_file  != NULL) __util_redirect(0 , stdin_file  , O_RDONLY);

    
    __argv        = util_malloc((argc + 2) * sizeof * __argv , __func__);  
    __argv[0]     = executable;
    for (iarg = 0; iarg < argc; iarg++)
      __argv[iarg+1] = argv[iarg];
    __argv[argc + 1] = NULL;

    /* 
       If executable is an absolute path, it is invoked directly, 
       otherwise PATH is used to locate the executable.
    */
    execvp( executable , (char **) __argv);
    /* 
       Exec should *NOT* return - if this code is executed
       the exec??? function has indeed returned, and this is
       an error.
    */
    util_abort("%s: failed to execute external command: \'%s\': %s \n",__func__ , executable , strerror(errno));
    
  }  else {
    /* Parent */
    if (blocking) {
      int child_status;
      waitpid(child_pid , &child_status , 0);
      
      if (target_file != NULL)
	if (!util_file_exists(target_file))
	  util_abort("%s: %s failed to produce target_file:%s aborting \n",__func__ , executable , target_file);
    }
  }

  util_safe_free( __argv );
  return child_pid;
}



/** 
    This function will TRY to aquire an exclusive lock to the file
    filename. If the file does not exist it will be created. The mode
    will be changed to 'mode' (irrespictive of whether it exists
    already or not).

    Observe that before the lockf() call we *MUST* succeed in opening
    the file, that means that if we do not have the necessary rights
    to open the file (with modes O_WRONLY + O_CREATE), the function
    will fail hard before even reaching the lockf system call.

    If the lock is aquired the function will return true, otherwise it
    will return false. The lock is only active as long as the lockfile
    is open, we therefor have to keep track of the relevant file
    descriptor; it is passed back to the calling scope through a
    reference. Observe that if the locking fails we close the file
    immediately, and return -1 in the file descriptor argument.

    When the calling scope is no longer interested in locking the
    resource it should close the file descriptor.

    ** Observe that with this locking scheme the existence of a lockfile
       is not really interesting. **
*/
    

bool util_try_lockf(const char * lockfile , mode_t mode , int * __fd) {
  int status;
  int lock_fd;
  lock_fd = open(lockfile , O_WRONLY + O_CREAT); 
  if (lock_fd == -1) 
    util_abort("%s: failed to open lockfile:%s %d/%s\n",__func__ , lockfile,errno , strerror(errno));

  fchmod(lock_fd , mode);
  status = lockf(lock_fd , F_TLOCK , 0);
  if (status == 0) {
    /* We got the lock for exclusive access - all is hunkadory.*/
    *__fd = lock_fd;
    return true;
  } else {
    if (errno == EACCES || errno == EAGAIN) {
      close(lock_fd);
      *__fd = -1;
      return false;
    } else {
      util_abort("%s: lockf() system call failed:%d/%s \n",__func__ , errno , strerror(errno));
      return false; /* Compiler shut up. */
    }
  }
}



static void  __add_item__(int **_active_list , int * _current_length , int *_list_length , int value) {
  int *active_list    = *_active_list;
  int  current_length = *_current_length;
  int  list_length    = *_list_length;

  active_list[current_length] = value;
  current_length++;
  if (current_length == list_length) {
    list_length *= 2;
    active_list  = util_realloc( active_list , list_length * sizeof * active_list , __func__);
    
    *_active_list = active_list;
    *_list_length = list_length;
  }
  *_current_length = current_length;
}


/**
   This function finds the current linenumber (by counting '\n'
   characters) in a currently open stream. It is an extremely
   primitive routine, and should only be used exceptionally.

   Observe that it implements "natural" counting, starting at line_nr
   1, and not line_nr 0.
*/

int util_get_current_linenr(FILE * stream) {
  long init_pos = ftell(stream);
  int line_nr   = 0;
  fseek( stream , 0L , SEEK_SET);
  {
    int char_nr;
    int c;
    for (char_nr = 0; char_nr < init_pos; char_nr++) {
      c = fgetc(stream);
      if (c == '\n')
	line_nr++;
    }
  }
  return line_nr;
}



/* 
   This functions parses an input string 'range_string' of the type:

     "0,1,8, 10 - 20 , 15,17-21"
 
   I.e. integers separated by "," and "-". The integer values are
   parsed out. The result can be returned in two different ways:


    o If active != NULL the entries in active (corresponding to the
      values in the range) are marked as true. All other entries are
      marked as false. The active array must be allocated by the
      calling scope, with length (at least) "max_value + 1".

    o If active == NULL - an (int *) pointer is allocated, and filled
      with the active indices.

*/

#include <stringlist.h>
#include <tokenizer.h>
static int * util_sscanf_active_range__NEW(const char * range_string , int max_value , bool * active , int * _list_length) {
  tokenizer_type * tokenizer = tokenizer_alloc( NULL  , /* No ordinary split characters. */
                                                NULL  , /* No quoters. */
                                                ",-"  , /* Special split on ',' and '-' */
                                                " \t" , /* Removing ' ' and '\t' */
                                                NULL  , /* No comment */
                                                NULL  );
  stringlist_type * tokens;
  tokens = tokenize_buffer( tokenizer , range_string , true);
  
  stringlist_free( tokens );
  tokenizer_free( tokenizer );
} 
   



static int * util_sscanf_active_range__(const char * range_string , int max_value , bool * active , int * _list_length) {
  int *active_list    = NULL;
  int  current_length = 0;
  int  list_length;
  int  value,value1,value2;
  char  * start_ptr = (char *) range_string;
  char  * end_ptr;
  
  if (active != NULL) {
    for (value = 0; value <= max_value; value++)
      active[value] = false;
  } else {
    list_length = 10;
    active_list = util_malloc( list_length * sizeof * active_list , __func__);
  }
    

  while (start_ptr != NULL) {
    value1 = strtol(start_ptr , &end_ptr , 10);
    if (active != NULL && value1 > max_value)
      fprintf(stderr , "** Warning - value:%d is larger than the maximum value: %d \n",value1 , max_value);
    
    if (end_ptr == start_ptr) 
      util_abort("%s: failed to parse integer from: %s \n",__func__ , start_ptr);
    
    /* OK - we have found the first integer, now there are three possibilities:
       
      1. The string contains nothing more (except) possibly whitespace.
      2. The next characters are " , " - with more or less whitespace.
      3. The next characters are " - " - with more or less whitespace.
    
    Otherwise it is a an invalid string.
    */


    /* Storing the value. */
    if (active != NULL) {
      if (value1 <= max_value) active[value1] = true;
    } else 
      __add_item__(&active_list , &current_length , &list_length , value1);



    /* Skipping trailing whitespace. */
    start_ptr = end_ptr;
    while (start_ptr[0] != '\0' && isspace(start_ptr[0]))
      start_ptr++;
    
    
    if (start_ptr[0] == '\0') /* We have found the end */
      start_ptr = NULL;
    else {
      /* OK - now we can point at "," or "-" - else malformed string. */
      if (start_ptr[0] == ',' || start_ptr[0] == '-') {
	if (start_ptr[0] == '-') {  /* This is a range */
	  start_ptr++; /* Skipping the "-" */
	  while (start_ptr[0] != '\0' && isspace(start_ptr[0]))
	    start_ptr++;
	  
	  if (start_ptr[0] == '\0') 
	    /* The range just ended - without second value. */
	    util_abort("%s[0]: malformed string: %s \n",__func__ , start_ptr);

	  value2 = strtol(start_ptr , &end_ptr , 10);
	  if (end_ptr == start_ptr) 
	    util_abort("%s[1]: failed to parse integer from: %s \n",__func__ , start_ptr);

	  if (active != NULL && value2 > max_value)
	    fprintf(stderr , "** Warning - value:%d is larger than the maximum value: %d \n",value2 , max_value);
	  
	  if (value2 < value1)
	    util_abort("%s[2]: invalid interval - must have increasing range \n",__func__);
	  
	  start_ptr = end_ptr;
	  { 
	    int value;
	    for (value = value1 + 1; value <= value2; value++) {
	      if (active != NULL) {
		if (value <= max_value) active[value] = true;
	      } else
		__add_item__(&active_list , &current_length , &list_length , value);
	    }
	  }
	  
	  /* Skipping trailing whitespace. */
	  while (start_ptr[0] != '\0' && isspace(start_ptr[0]))
	    start_ptr++;
	  
	  
	  if (start_ptr[0] == '\0')
	    start_ptr = NULL; /* We are done */
	  else {
	    if (start_ptr[0] == ',')
	      start_ptr++;
	    else
	      util_abort("%s[3]: malformed string: %s \n",__func__ , start_ptr);
	  }
	} else 
	  start_ptr++;  /* Skipping "," */

	/**
	   When this loop is finished the start_ptr should point at a
	   valid integer. I.e. for instance for the following input
	   string:  "1-3 , 78"
	                   ^
			   
	   The start_ptr should point at "78".
	*/

      } else 
	util_abort("%s[4]: malformed string: %s \n",__func__ , start_ptr);
    }
  }
  if (_list_length != NULL)
    *_list_length = current_length;

  return active_list;
}


void util_sscanf_active_range(const char * range_string , int max_value , bool * active) {
  util_sscanf_active_range__(range_string , max_value , active , NULL);
}

int * util_sscanf_alloc_active_list(const char * range_string , int * list_length) {
  return util_sscanf_active_range__(range_string , 0 , NULL , list_length);
}


/**
   This function updates an environment variable representing a path,
   i.e. ":" separated. 
*/

void util_update_path_var(const char * variable, const char * value, bool append) {
  if (getenv(variable) == NULL)
    /* The (path) variable is not currently set. */
    setenv( variable , value , 1);
  else {
    char * new_value;

    if (append)
      new_value = util_alloc_sprintf("%s:%s" , getenv( variable ) , value);
    else
      new_value = util_alloc_sprintf("%s:%s" , value , getenv( variable ));

    //unsetenv( variable );
    setenv( variable , new_value , 1);
    free( new_value);
  }
}



#include "util_path.c"

