#include <string.h>
#include <util.h>
#include <hash.h>
#include <set.h>
#include <vector.h>
#include <int_vector.h>
#include <conf.h>
#include <tokenizer.h>



struct spec_struct
{
  char                 * name;
  const spec_type      * super;
  hash_type            * specs;
  validator_ftype      * validator;
};



struct conf_struct
{
  char        * type;
  char        * name;

  vector_type * confs;
  hash_type   * items;
};



struct conf_item_struct
{
  stringlist_type * values;
};



/******************************************************************************/
/*                              STATIC HEADERS                                */
/******************************************************************************/
//static
//int conf_add_data_from_file(
//  conf_type             * conf,         /** The conf_type data is added to.   */
//  const set_type        * src_files,    /** Files sourced higher in tree.     */
//  const char            * current_file  /** The file currently beeing parsed. */
//);
/******************************************************************************/


static
bool strings_are_equal(
  const char * string_a,
  const char * string_b)
{
  if( strcmp(string_a, string_b) == 0)
    return true;
  else
    return false;
}





static
bool string_is_special(
  const char * str)
{
  if(      strings_are_equal(str, "{") )
    return true;
  else if( strings_are_equal(str, "}") )
    return true;
  else if( strings_are_equal(str, "[") )
    return true;
  else if( strings_are_equal(str, "]") )
    return true;
  else if( strings_are_equal(str, "=") )
    return true;
  else
    return false;
}


static
bool string_is_include(
  const char * str)
{
  return strings_are_equal(str, "include");
}



static
conf_type * conf_alloc_empty(
  const char * type,
  const char * name)
{
  return NULL;
}



static
stringlist_type * get_tokens_from_file(
  const char * filename                 /** File to tokenize. */
)
{
  /**
    TODO

    Implement me.
  */

  return NULL;
}


static
void append_unexpected_token_error(
  const char            * current_file, /** The file currently being parsed.  */
  const stringlist_type * tokens,       /** Tokens created from the file.     */
  int                     position,     /** Position of the unexpected token. */
  stringlist_type       * errors        /** Storage for error messages.       */
)      
{
  /**
    TODO

    Implement me.
  */
}



static
void append_unexpected_end_of_file_error(
  const char            * current_file, /** The file currently being parsed.  */
  const stringlist_type * tokens,       /** Tokens created from the file.     */
  stringlist_type       * errors        /** Storage for error messages.       */
)
{
  /**
    TODO

    Implement me.
  */
}





/**
  This function will add ALL data from the stringlist "tokens to conf.

  Thus, it is the callers responsbility to enure that 
  
*/
//static
//int conf_add_data_from_tokens(
//  conf_type             * conf,         /** The conf_type data is added to.   */
//  const set_type        * src_files,    /** Files sourced higher in tree.     */
//  char                  * current_file, /** The file currently being sourced. */
//  const stringlist_type * tokens,       /** Tokens created from the file.     */
//  stringlist_type       * errors        /** Storage for error messages.       */
//)
//{
//  int position = 0;
//  int num_tokens = stringlist_get_size(tokens);
//
//  while(position < num_tokens)
//  {
//    /**
//      On entering this loop, stringlist_iget(tokens, position)
//      shall always be a non-special character. Everything
//      else is a syntax error.
//    */
//    if( string_is_special(stringlist_iget(tokens, position)) )
//    {
//      append_unexpected_token_error(current_file, tokens, position, errors);
//      return CONF_PARSE_ERROR;
//    }
//
//
//
//    /**
//      Check if the token is an include statement, and handle it if so.
//    */
//    if( string_is_include(stringlist_iget(tokens, position)) )
//    {
//      position++;
//      if( position == num_tokens )
//      {
//        append_unexpected_end_of_file_error(current_file, tokens, errors);
//        return CONF_PARSE_ERROR;
//      }
//      else
//      {
//        int status;
//        const char * filename = stringlist_iget(tokens, position);
//        set_type   * src_files_new = set_copyc(src_files);
//        char       * rp_current_file = util_alloc_realpath(current_file);
//
//        set_add_key(src_files_new, rp_current_file);
//        free(rp_current_file);
//        status = conf_add_data_from_file(conf, src_files, filename, errors);
//        set_free(src_files_new);
//
//        if( status != CONF_OK )
//          return status;
//      }
//      position++;
//      continue;
//    }
//
//
//
//    /**
//      If we are here, we are guaranteed that the token is an item or a new
//      conf group. The rule to decide if it's an item or a group is to check
//      if the next token is an assigment (i.e. a '='). If the token is an
//      assigment, we have an item. Otherwise, we have a group.
//    */
//    {
//      const char * name =  stringlist_iget(tokens, position);
//      position++;
//
//      if(position == num_tokens)
//      {
//          append_unexpected_end_of_file_error(current_file, tokens, errors);
//          return CONF_PARSE_ERROR;
//      }
//
//      if( strings_are_equal(stringlist_iget(tokens, position), "=" ) )
//      {
//        /**
//          OK, it was an item. Parse it.
//        */
//
//      }
//      else
//      {
//        /**
//          OK, it was a group. Parse it.
//        */
//
//      }
//    }
//
//
//  }
//
//  return CONF_OK;
//}



static
int create_token_buffer__(
  stringlist_type ** tokens,      /** Memory to contain the created pointer.  */
  stringlist_type ** src_files,   /** Memory to contain the created pointer.  */
  set_type         * sources,     /** Realpath of files sourced previously .  */
  tokenizer_type   * tokenizer,   /** Tokenizer to use.                       */
  const char       * filename     /** File to tokenize.                       */
)
{
  stringlist_type * tokens__;
  stringlist_type * src_files__;
  char * realpath_filename = util_alloc_realpath(filename);
  
  int status;
  if( !set_has_key(sources, realpath_filename) )
    status = CONF_OK;
  else
    status = CONF_RECURSIVE_INCLUDE_ERROR;

  if( status == CONF_OK )
  {
    set_type * sources__   = set_copyc(sources);
    set_add_key(sources__, realpath_filename);

    tokens__    = tokenize_file(tokenizer, filename, true);
    src_files__ = stringlist_alloc_new(); 

    /**
      Set the sources. Let the first token own the memory.
    */
    for(int i=0; i<stringlist_get_size(tokens__); i++)
    {
      if(i == 0)
        stringlist_append_copy(src_files__, realpath_filename);
      else
        stringlist_append_ref(src_files__, stringlist_iget(src_files__, 0));
    }

    /**
      Handle any include statements by recursion.
    */
    {
      int i = stringlist_find_first(tokens, "include");
      while( i != -1)
      {
        /**
          Check that the next token actually exists.
        */
        if( i+1 == stringlist_get_size(tokens) )
        {
          status = CONF_UNEXPECTED_EOF_ERROR;
          break;
        }

        i = stringlist_find_first(tokens, "include");
      }
    }

    set_free(sources);
  }


  free(realpath_filename);

  if( status == CONF_OK )
  {
    *tokens    = tokens__;
    *src_files = src_files__;
  }
  else
  {
    *tokens    = NULL;
    *src_files = NULL;
  }
  return status;
}



static
int create_token_buffer(
  stringlist_type ** tokens,
  stringlist_type ** src_files,
  const char       * filename
)
{
  tokenizer_type * tokenizer = tokenizer_alloc(" \t\n\r,;", "'\"", "[]{}",
                                               "--", "\n");
  stringlist_type * tokens__   ; 
  stringlist_type * src_files__;

  set_type        * sources   = set_alloc_empty();

  int status = create_token_buffer__(&tokens__, &src_files__, sources,
                                     tokenizer, filename);
  tokenizer_free(tokenizer);

  *tokens    = tokens__;
  *src_files = src_files__;

  return status;
}
