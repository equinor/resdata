#include <string.h>
#include <util.h>
#include <hash.h>
#include <set.h>
#include <vector.h>
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
/* STATIC HEADERS.*/

/******************************************************************************/
static
int conf_add_data_from_file(
  conf_type             * conf,         /** The conf_type data is added to.   */
  const set_type        * src_files,    /** Files sourced higher in tree.     */
  const char            * current_file  /** The file currently beeing parsed. */
);




static
bool string_is_special(
  const char * str)
{
  if(      strcmp(str, "{") == 0 )
    return true;
  else if( strcmp(str, "}") == 0 )
    return true;
  else if( strcmp(str, "[") == 0 )
    return true;
  else if( strcmp(str, "]") == 0 )
    return true;
  else if( strcmp(str, "=") == 0 )
    return true;
  else
    return false;
}


static
bool string_is_include(
  const char * str)
{
  if( strcmp(str, "include") == 0 )
    return true;
  else
    return false;
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




static
int conf_add_data_from_tokens(
  conf_type             * conf,         /** The conf_type data is added to.   */
  const set_type        * src_files,    /** Files sourced.                    */
  char                  * current_file, /** The file currently being sourced. */
  const stringlist_type * tokens,       /** Tokens created from the file.     */
  stringlist_type       * errors        /** Storeage for error messages.      */
)
{
  int position = 0;
  int num_tokens = stringlist_get_size(tokens);

  /**
    On entering this loop, stringlist_iget(position)
    shall always be a non-special character. Everything
    else is a syntax error.
  */
  while(position < num_tokens)
  {
    if( string_is_special(stringlist_iget(tokens, position)) )
    {
      append_unexpected_token_error(current_file, tokens, position, errors);
      return CONF_PARSE_ERROR;
    }
    if( string_is_include(stringlist_iget(tokens, position)) )
    {
      position++;
      if( position == num_tokens )
      {
        append_unexpected_end_of_file_error(current_file, tokens, errors);
        return CONF_PARSE_ERROR;
      }
      else
      {
        const char * filename = stringlist_iget(tokens, position);
        int status = conf_add_data_from_file(conf, src_files, filename);
        
        if( status != CONF_OK )
          return status;
      }
      position++;
    }

  }

  return CONF_OK;
}



static
int conf_add_data_from_file(
  conf_type      * conf,         /** The conf_type data is added to.   */
  const set_type * src_files,    /** Files sourced higher in tree.     */
  const char     * current_file  /** The file currently beeing parsed. */
)
{
  return CONF_OK;
}
