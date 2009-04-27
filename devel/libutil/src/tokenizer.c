#include <assert.h>
#include <string.h>
#include <util.h>
#include <tokenizer.h>

#define TOKENIZER_ESCAPE_CHAR '\\'


/** These items are documented in the header file tokenizer.h */ 
struct tokenizer_struct
{
  char * whitespace;    
  char * quoters;       
  char * specials;      
  char * comment_start; 
  char * comment_end;   
  char * stop_set;
};



tokenizer_type * tokenizer_alloc(
  const char * whitespace,       /** Set to NULL if not interessting.            */
  const char * quoters,          /** Set to NULL if not interessting.            */
  const char * specials,         /** Set to NULL if not interessting.            */
  const char * comment_start,    /** Set to NULL if not interessting.            */
  const char * comment_end)      /** Set to NULL  if not interessting.           */
{
  tokenizer_type * tokenizer = util_malloc(sizeof * tokenizer, __func__);
  

  if( whitespace != NULL)
  {
    if( strlen(whitespace) == 0)
      util_abort("%s: Need at least one non '\\0' whitespace character.\n", __func__);
    tokenizer->whitespace = util_alloc_string_copy( whitespace ); 
  }
  else
    tokenizer->whitespace = NULL;

  if( quoters != NULL )
  {
    if( strlen( quoters ) == 0)
      util_abort("%s: Need at least one non '\\0' quote character.\n", __func__);
    tokenizer->quoters = util_alloc_string_copy( quoters ); 
  }
  else
    tokenizer->quoters = NULL;

  if( specials != NULL )
  {
    if( strlen( specials ) == 0)
      util_abort("%s: Need at least one non '\\0' special character.\n", __func__);
    tokenizer->specials = util_alloc_string_copy( specials ); 
  }
  else
    tokenizer->specials = NULL;

  if( comment_start != NULL )
  {
    if( strlen( comment_start ) == 0)
      util_abort("%s: Need at least one non '\\0' character to start a comment.\n", __func__);
    tokenizer->comment_start = util_alloc_string_copy( comment_start );
  }
  else
    tokenizer->comment_start = NULL;
    
  if( comment_end != NULL )
  {
    if( strlen( comment_end ) == 0)
      util_abort("%s: Need at least one non '\\0' character to end a comment.\n", __func__);
    tokenizer->comment_end   = util_alloc_string_copy( comment_end );
  }
  else 
    tokenizer->comment_end   = NULL;

  if(comment_start == NULL && comment_end != NULL)
    util_abort("%s: Need to have comment_start when comment_end is set.\n", __func__);
  if(comment_start != NULL && comment_end == NULL)
    util_abort("%s: Need to have comment_end when comment_start is set.\n", __func__);
  
  tokenizer->stop_set = NULL;

  return tokenizer;
}



void tokenizer_free(
  tokenizer_type * tokenizer)
{
 free( tokenizer->whitespace    );

 util_safe_free( tokenizer->quoters       ); 
 util_safe_free( tokenizer->specials      ); 
 util_safe_free( tokenizer->comment_start );
 util_safe_free( tokenizer->comment_end   );
 util_safe_free( tokenizer->stop_set      );
 free( tokenizer     );
}



static
bool is_escape(
  const char c)
{
  if( c == TOKENIZER_ESCAPE_CHAR )
    return true;
  else
    return false;
}



static
int length_of_initial_whitespace(
  const char           * buffer_position,
  const tokenizer_type * tokenizer)
{
  assert( buffer_position != NULL );
  assert( tokenizer       != NULL );

  if( tokenizer->whitespace == NULL)
    return 0;
  else
    return strspn( buffer_position, tokenizer->whitespace );
}



static
bool is_whitespace(
  const char             c,
  const tokenizer_type * tokenizer)
{
  if( tokenizer->whitespace == NULL)
    return false;
  else if( strchr( tokenizer->whitespace, (int) c) != NULL)
    return true;
  else
    return false;
}



static 
bool is_stop_char(const char c,
			 const tokenizer_type * tokenizer) {
  if (tokenizer->stop_set == NULL)
    return false;
  else if( strchr( tokenizer->stop_set, (int) c) != NULL)
    return true;
  else
    return false;
}



static
bool is_special(
  const char             c,
  const tokenizer_type * tokenizer)
{
  if( tokenizer->specials == NULL)
    return false;
  else if( strchr( tokenizer->specials, (int) c) != NULL)
    return true;
  else
    return false;
}



static
bool is_in_quoters(
  const char       c,
  const tokenizer_type * tokenizer)
{
  bool in_set = false;
  if(tokenizer->quoters == NULL)
    in_set = false;
  else
  {
    const char * char_pos = strchr( tokenizer->quoters, (int) c);
    if( char_pos != NULL)
      in_set = true;
    else
      in_set = false;
  }
    
  return in_set;
}



/**
  This finds the number of characters up til and including the next
  occurence of buffer[0].

  E.g. using this funciton on

  char * example = "1231abcd";

  should return 4.

  If the character can not be found, it will return the length of the
  buffer.

  Escaped occurences of the first character are not counted. E.g. if
  TOKENIZER_ESCAPE_CHAR occurs in front of a new occurence of the
  first character, this is *NOT* regarded as the end.
  
*/
static
int length_of_quotation(
  const char * buffer)
{
  assert( buffer != NULL );

  int  length  = 1;
  char target  = buffer[0];
  char current = buffer[1]; 

  bool escaped = false;
  while(current != '\0' &&  !(current == target && !escaped ))
  {
    escaped = is_escape(current);
    length += 1;
    current = buffer[length];
  }
  length += 1;

  return length;
}



static
int length_of_comment(
  const char           * buffer_position,
  const tokenizer_type * tokenizer)
{  
  bool in_comment = false;
  int length = 0;

  if(tokenizer->comment_start == NULL || tokenizer->comment_end == NULL)
    length = 0;
  else
  {
    const char * comment_start     = tokenizer->comment_start;
    int          len_comment_start = strlen( comment_start );
    if( strncmp( buffer_position, comment_start, len_comment_start) == 0)
    {
      in_comment = true;
      length     = len_comment_start;
    }
    else
      length = 0;
  }

  if( in_comment )
  {
    const char * comment_end       = tokenizer->comment_end;
    int          len_comment_end   = strlen( comment_end   );
    while(buffer_position[length] != '\0' && in_comment)
    {
      if( strncmp( &buffer_position[length], comment_end, len_comment_end) == 0)
      {
        in_comment = false;
        length += len_comment_end; 
      }
      else
        length += 1;
    }
  }
  return length;
}



static
char * alloc_quoted_token(
  const char * buffer,
  int          length,
  bool         strip_quote_marks)
{
  char * token;
  if(!strip_quote_marks)
  {
    token = util_malloc( (length + 1) * sizeof * token, __func__ );
    memmove(token, &buffer[0], length * sizeof * token );
    token[length] = '\0';
  }
  else
  {
    token = util_malloc( (length - 1) * sizeof * token, __func__ );
    memmove(token, &buffer[1], (length -1) * sizeof * token);
    token[length-2] = '\0';
    /**
      Removed escape char before any escaped quotation starts.
    */
    {
      int  new_length = length-1;
      char expr[3];
      char subs[2];
      expr[0] = TOKENIZER_ESCAPE_CHAR;
      expr[1] = buffer[0];
      expr[2] = '\0';
      subs[0] = buffer[0];
      subs[1] = '\0';
      util_string_replace_inplace(&token, &new_length, expr, subs);
    }
  }
  return token;
}



static
int length_of_normal_non_whitespace(
  const char           * buffer,
  const tokenizer_type * tokenizer)
{
  bool at_end  = false;
  int length   = 0;
  char current = buffer[0];

  while(current != '\0' && !at_end)
  {
    length += 1;
    current = buffer[length];

    if( is_whitespace( current, tokenizer ) )
    {
      at_end = true;
      continue;
    }
    if( is_special( current, tokenizer ) )
    {
      at_end = true;
      continue;
    }
    if( is_in_quoters( current, tokenizer ) )
    {
      at_end = true;
      continue;
    }
    if( length_of_comment(&buffer[length], tokenizer) > 0)
    {
      at_end = true;
      continue;
    }
  }

  return length;
}


/**

   This is the lowest level tokenizer - which actually does the
   job. This function will tokenize until either the buffer is
   exhausted, or characters in the stop_set are encountered. The
   return value is the number of characters actually parsed from the
   buffer.

   Typical usage:

   stringlist_type * tokens = stringlist_alloc_new();
   int buffer_position = 0;
   while (buffer_position < buffer_size) {
       buffer_position += tokenizer_buffer__( tokenizer , &buffer[buffer_position] , strip_quote_marks , tokens);
       //Do something with the tokens 
   }
   stringlist_free( tokens );

   
*/




static int tokenize_buffer__(const tokenizer_type * tokenizer , const char * buffer , bool strip_quote_marks, stringlist_type * tokens) {
  int  buffer_size = strlen( buffer );
  int  position = 0;
  bool complete = false;
  stringlist_clear( tokens );
  while( !complete ) {
    if (position >= buffer_size) {
      complete = true;
      continue;
    }
    
    /** 
	Skip initial whitespace.
    */
    {
      int whitespace_length = length_of_initial_whitespace( &buffer[position], tokenizer );
      if(whitespace_length > 0) {
	position += whitespace_length;
	continue;
      }
    }
    
    
    /**
       Skip comments.
    */
    {
      int comment_length = length_of_comment( &buffer[position], tokenizer);
      if(comment_length > 0) {
	position += comment_length;
	continue;
      }
    }
    
    

    /** 
	Copy the character if it is in the special set,
    */
    if( is_special( buffer[position], tokenizer ) )
      {
	char key[2];
	key[0] = buffer[position];
	key[1] = '\0';
	stringlist_append_copy( tokens, key );
	position += 1;
	continue;
      }
    
    /**
       If the character is a quotation start,
       we copy the whole quotation.
    */
    if( is_in_quoters( buffer[position], tokenizer ) )
      {
	int length = length_of_quotation( &buffer[position] );
	char * token = alloc_quoted_token( &buffer[position], length, strip_quote_marks );
	stringlist_append_owned_ref( tokens, token );
	position += length;
	continue;
      }
    
    
    /**
       If we are here, we are guaranteed that that buffer[position] is
       not:
       
       1. Whitespace.
       2. The start of a comment.
       3. A special character.
       4. The start of a quotation.
       
       In other words, it is the start of plain non-whitespace. Now we
       need to find the length of the non-whitespace until:
       
       1. Whitespace starts.
       2. A comment starts.
       3. A special character occur.
       4. A quotation starts.
    */
    
    {
      int length = length_of_normal_non_whitespace( &buffer[position], tokenizer );
      char * token = util_malloc( (length + 1) * sizeof * token, __func__ );
      memmove(token, &buffer[position], length * sizeof * token );
      token[length] = '\0';
      stringlist_append_owned_ref( tokens, token );
      position += length;
      continue;
    }
  }
  return position;
}



stringlist_type * tokenize_buffer(
  const tokenizer_type * tokenizer,
  const char           * buffer,
  bool                   strip_quote_marks)
{
  bool complete          = false;
  int  position          = 0;
  int  buffer_size       = strlen(buffer);

  
  stringlist_type * tokens = stringlist_alloc_new();
  position += tokenize_buffer__(tokenizer , buffer , strip_quote_marks , tokens);
  return tokens;

}



stringlist_type * tokenize_file(
  const tokenizer_type * tokenizer,
  const char           * filename,
  bool                   strip_quote_marks)
{
  stringlist_type * tokens;
  char * buffer = util_fread_alloc_file_content( filename, NULL, NULL );
  tokens = tokenize_buffer( tokenizer, buffer, strip_quote_marks );
  free(buffer);
  return tokens;
} 


