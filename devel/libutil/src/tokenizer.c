#include <assert.h>
#include <string.h>
#include <util.h>
#include <tokenizer.h>

#define TOKENIZER_ESCAPE_CHAR '\\'



struct tokenizer_struct
{
  char * splitters;         /* The string is split into tokens on the occurence of one of these characters - and they are removed. */
  char * specials;          /* This exactly like the splitters - but these characters are retained as tokens. */
  char * delete_set;        /* The chracters are just plain removed - but without any splitting on them. */
  char * quoters;       
  char * comment_start; 
  char * comment_end;   
};



tokenizer_type * tokenizer_alloc(
  const char * splitters,        /** Set to NULL if not interessting.            */
  const char * quoters,          /** Set to NULL if not interessting.            */
  const char * specials,         /** Set to NULL if not interessting.            */
  const char * delete_set,
  const char * comment_start,    /** Set to NULL if not interessting.            */
  const char * comment_end)      /** Set to NULL  if not interessting.           */
{
  tokenizer_type * tokenizer = util_malloc(sizeof * tokenizer, __func__);


  if( splitters != NULL)
  {
    if( strlen(splitters) == 0)
      util_abort("%s: Need at least one non '\\0' splitters character.\n", __func__);
    tokenizer->splitters = util_alloc_string_copy( splitters ); 
  }
  else
    tokenizer->splitters = NULL;
  
  if (delete_set != NULL)
    tokenizer->delete_set = util_alloc_string_copy( delete_set );
  else
    tokenizer->delete_set = NULL;


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
  

  return tokenizer;
}



void tokenizer_free(
  tokenizer_type * tokenizer)
{

  util_safe_free( tokenizer->splitters    );
  util_safe_free( tokenizer->quoters       ); 
  util_safe_free( tokenizer->specials      ); 
  util_safe_free( tokenizer->comment_start );
  util_safe_free( tokenizer->comment_end   );
  util_safe_free( tokenizer->delete_set    );

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
int length_of_initial_splitters(
  const char           * buffer_position,
  const tokenizer_type * tokenizer)
{
  assert( buffer_position != NULL );
  assert( tokenizer       != NULL );

  if( tokenizer->splitters == NULL)
    return 0;
  else
    return strspn( buffer_position, tokenizer->splitters );
}

static bool in_set(char c , const char * set) {
  if (set == NULL)
    return false;
  else {
    if (strchr( set , (int) c) != NULL)
      return true;
    else
      return false;
  }
}


static
bool is_splitters(
  const char             c,
  const tokenizer_type * tokenizer)
{
  return in_set(c , tokenizer->splitters);
}

static 
bool is_special(
  const char             c,
  const tokenizer_type * tokenizer)
{
  return in_set(c , tokenizer->specials);
}


static
bool is_in_quoters(
  const char       c,
  const tokenizer_type * tokenizer)
{
  return in_set(c , tokenizer->quoters);
}



static bool is_in_delete_set(const char c , const tokenizer_type * tokenizer) {
  return in_set(c , tokenizer->delete_set);
}


/**
  This finds the number of characters up til
  and including the next occurence of buffer[0].

  E.g. using this funciton on

  char * example = "1231abcd";

  should return 4.

  If the character can not be found, the function will fail with
  util_abort() - all quotation should be terminated (Joakim - WITH
  moustache ...). Observe that this function does not allow for mixed
  quotations, i.e. both ' and " might be vald as quaotation
  characters; but one quoted string must be wholly quoted with EITHER
  ' or ".

  Escaped occurences of the first character are
  not counted. E.g. if TOKENIZER_ESCAPE_CHAR
  occurs in front of a new occurence of the first
  character, this is *NOT* regarded as the end.
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

  if ( current == '\0') /* We ran through the whole string without finding the end of the quotation - abort HARD. */
    util_abort("%s: could not find quotation closing on %s \n",__func__ , buffer);
  
  
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




/** 
    This does not care about the possible occurence of characters in
    the delete_set. That is handled when the token is inserted in the
    token list.
*/
    
static
int length_of_normal_non_splitters(
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

    if( is_splitters( current, tokenizer ) )
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



static int length_of_delete( const char * buffer , const tokenizer_type * tokenizer) {
  int length   = 0;
  char current = buffer[0];

  while(is_in_delete_set( current , tokenizer ) && current != '\0') {
    length += 1;
    current = buffer[length];
  }
  return length;
}


/**
   Allocates a new stringlist. 
*/
stringlist_type * tokenize_buffer(
  const tokenizer_type * tokenizer,
  const char           * buffer,
  bool                   strip_quote_marks)
{
  int position          = 0;
  int buffer_size       = strlen(buffer);
  int splitters_length  = 0;
  int comment_length    = 0;
  int delete_length     = 0;

  stringlist_type * tokens = stringlist_alloc_new();

  while( position < buffer_size )
  {
    /** 
      Skip initial splitters.
    */
    splitters_length = length_of_initial_splitters( &buffer[position], tokenizer );
    if(splitters_length > 0)
    {
      position += splitters_length;
      continue;
    }


    /**
      Skip comments.
    */
    comment_length = length_of_comment( &buffer[position], tokenizer);
    if(comment_length > 0)
    {
      position += comment_length;
      continue;
    }

    
    /**
       Skip characters which are just deleted. 
    */
      
    delete_length = length_of_delete( &buffer[position] , tokenizer );
    if (delete_length > 0) {
      position += delete_length;
      continue;
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
       If the character is a quotation start, we copy the whole quotation.
    */
    if( is_in_quoters( buffer[position], tokenizer ) )
    {
      int length   = length_of_quotation( &buffer[position] );
      char * token = alloc_quoted_token( &buffer[position], length, strip_quote_marks );
      stringlist_append_owned_ref( tokens, token );
      position += length;
      continue;
    }

    /**
      If we are here, we are guaranteed that that
      buffer[position] is not:

      1. Whitespace.
      2. The start of a comment.
      3. A special character.
      4. The start of a quotation.
      5. Something to delete.

      In other words, it is the start of plain
      non-splitters. Now we need to find the
      length of the non-splitters until:

      1. Whitespace starts.
      2. A comment starts.
      3. A special character occur.
      4. A quotation starts.
    */

    {
      int length   = length_of_normal_non_splitters( &buffer[position], tokenizer );
      char * token = util_malloc( (length + 1) * sizeof * token, __func__ );
      int token_length;
      if (tokenizer->delete_set == NULL) {
        token_length = length;
        memcpy( token , &buffer[position] , length * sizeof * token );
      } else {
        int i;
        token_length = 0;
        for (i = 0; i < length; i++) {
          char c = buffer[position + i];
          if ( !is_in_delete_set( c , tokenizer)) {
            token[token_length] = c;
            token_length++;
          }
        }
      }


      if (token_length > 0) { /* We do not insert empty tokens. */
        token[token_length] = '\0';
        stringlist_append_owned_ref( tokens, token );
      } else 
        free( token );    /* The whole thing is discarded. */

      position += length;
      continue;
    }
  }

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

