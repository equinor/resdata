#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>



char * strdup_n(const char *s , int max_len) {
  char *new_string;
  int len;

  if (strlen(s) > max_len)
    len = max_len;
  else
    len = strlen(s);
  new_string = malloc(len + 1);
  
  strncpy(new_string , s , len);  new_string[len] = '\0';
  return new_string;
}



char * strip_line_alloc(const char * line) {
  const char  comment_char = '-';
  const char *space   = " \t";
  char * new_line = NULL;
  int offset, length,pos;
  bool cont , quote_on , at_end;
  
  offset   = strspn(line , space);
  quote_on = false;
  cont     = true;
  at_end   = false;
  if (line[offset] != '\0') {
    pos = offset;
    do {
      if (line[pos] == '\'' || line[pos] == '"')
	quote_on = !quote_on;

      if (line[pos] == comment_char && !quote_on) {
	cont   = false;
	length = pos - offset;
      }
      
      if (pos == (strlen(line) - 1)) {
	length = pos - offset + 1;
	cont = false;
	at_end = true;
      }
      if (cont)
	pos++;
    } while (cont);
  
    /*if (at_end) {} remove trailing space*/
      
  
    if (length > 0)
      new_line = strdup_n(&line[offset] , length);
  } 
  
  return new_line;
}


char * alloc_line(FILE *stream , bool *at_eof) {
  int init_pos = ftell(stream);
  int len;
  char *line;
  char c , end_char;
  bool cont;
  bool dos_newline;
  
  len = 0;
  cont = true;
  
  
  do {
    c = fgetc(stream);
    if (c != '\n' && c != '\r' && c != EOF)
      len++;
    else
      cont = false;
  } while (cont);
  if (c == '\r')
    dos_newline = true;
  else
    dos_newline = false;
  end_char = c;

  fseek(stream , init_pos , SEEK_SET);
  {
    char *tmp_line = malloc(len + 1);
    int i;
    for (i=0; i < len; i++)
      tmp_line[i] = fgetc(stream);
    tmp_line[len] = '\0';
    line = strip_line_alloc(tmp_line);
    free(tmp_line);
  }
  fgetc(stream);
  if (dos_newline)
    fgetc(stream);
  
  if (end_char == EOF)
    *at_eof = true;
  else
    *at_eof = false;

  return line;
}




void parse_file(const char *filename , int *_lines , char ***_line_list) {
  int    lines;
  char **line_list;
  int    buffer_lines;
  bool   at_eof;
  FILE *stream = fopen(filename , "r");
  
  lines        = 0;
  buffer_lines = 100;
  line_list    = NULL;
  
  line_list = realloc(line_list , buffer_lines * sizeof *line_list);
  do {
    char * line = alloc_line(stream , &at_eof);
    if (line != NULL) {
      line_list[lines] = line;
      lines++;

      if (lines == buffer_lines) {
	buffer_lines *= 2;
	line_list = realloc(line_list , buffer_lines * sizeof *line_list);
      }
    }
  } while (!at_eof);
  fclose(stream);

  line_list = realloc(line_list , lines * sizeof *line_list);
  *_lines     = lines;
  *_line_list = line_list;
}



void parse_line(const char * line , int *_tokens , char ***_token_list , int min_tokens) {
  const char *delimiters = " ";
  int    token,tokens,offset,length;
  char **token_list;
  
  tokens  = 0;
  offset = strspn(line , delimiters);

  while (line[offset ] != '\0') {
    length = strcspn(&line[offset] , delimiters);
    if (length > 0) {
      char * token_string = strdup_n(&line[offset] , length);
      char * star_ptr     = token_string;
      long int items      = strtol(token_string , &star_ptr , 10);
      
      if (star_ptr != token_string && star_ptr[0] == '*') 
	tokens += items;
      else
	tokens++;
      
      offset += length;                              /* Skipping the token */
      offset += strspn(&line[offset] , delimiters);  /* Skipping the following whitespace */

      free(token_string);
    }
  }
  
  if (tokens < min_tokens)
    tokens = min_tokens;
  token_list = malloc(tokens * sizeof *token_list);
  for (token = 0; token < tokens; token++) 
    token_list[token] = NULL;
  
  token   = 0;
  offset = strspn(line , delimiters);
  while (line[offset ] != '\0') {
    length = strcspn(&line[offset] , delimiters);
    if (length > 0) {
      char * token_string  = strdup_n(&line[offset] , length);
      char * star_ptr      = token_string;
      long int items = strtol(token_string , &star_ptr , 10);
      
      if (star_ptr != token_string && star_ptr[0] == '*') {
	int t1 = token;
	int t2 = token + items;
	    
	for (token = t1; token < t2; token++) {
	  if (strlen(star_ptr) == 1)
	    token_list[token] = NULL;
	  else
	    token_list[token] = strdup(&star_ptr[1]);
	}
      } else {
	token_list[token] = strdup_n(&line[offset] , length);
	token++;
      }
      
      offset += length;                              /* Skipping the token */
      offset += strspn(&line[offset] , delimiters);  /* Skipping the following whitespace */
      free(token_string);
    }
  }
  
  /*
    Removing quotes ...
  */
  for (token = 0; token < tokens; token++) {
    if (token_list[token] != NULL) {
      if (token_list[token][0] == '\'') {
	char *tmp = strdup_n(&token_list[token][1] , strlen(token_list[token]) - 2);
	free(token_list[token]);
	token_list[token] = tmp;
      }
    }
  }

  *_tokens = tokens;
  *_token_list = token_list;
}


void sch_parse_wconhist(int lines , const char **line_list) {
  bool wconhist_on;
  int line;

  wconhist_on = false;
  for (line = 0; line < lines; line++) {
    if (!wconhist_on) {
      if (strncmp(line_list[line] , "WCONHIST" , 8) == 0) 
	wconhist_on = true;
    } else {
      if (strcmp(line_list[line] , "/") == 0) {
	wconhist_on = false;
	printf("\n");
      } else {
	char **token_list;
	int tokens;
	parse_line(line_list[line] , &tokens , &token_list , 0);
	printf("%s  %g  %g  %g\n",token_list[0] , atof(token_list[3]) , atof(token_list[4]) , atof(token_list[5]));
      }
    }
  }
}


int main(void) {
  int tokens, token;
  char **token_list;
  {
    int lines , i;
    char **line_list;
    parse_file("SCHEDULE_orig.INC" , &lines , &line_list);
    sch_parse_wconhist(lines , (const char **) line_list);
  }
  return 0;
}
