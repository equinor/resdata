#include <assert.h>
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



//static
//conf_type * conf_alloc_empty(
//  const char * type,
//  const char * name)
//{
//  return NULL;
//}
//
//
//
//static
//void append_unexpected_token_error(
//  const char            * current_file, /** The file currently being parsed.  */
//  const stringlist_type * tokens,       /** Tokens created from the file.     */
//  int                     position,     /** Position of the unexpected token. */
//  stringlist_type       * errors        /** Storage for error messages.       */
//)      
//{
//  /**
//    TODO
//
//    Implement me.
//  */
//}
//
//
//
//static
//void append_unexpected_end_of_file_error(
//  const char            * current_file, /** The file currently being parsed.  */
//  const stringlist_type * tokens,       /** Tokens created from the file.     */
//  stringlist_type       * errors        /** Storage for error messages.       */
//)
//{
//  /**
//    TODO
//
//    Implement me.
//  */
//}



/**
  Find the realpath of file included
  from another file.
*/
static
int alloc_included_realpath(
  const char  * current_file_realpath,
  const char  * include_file,
  char       ** include_file_realpath
)
{
  assert(current_file_realpath != NULL);
  assert(include_file          != NULL);

  bool realpath_ok;
  char * path;

  if( util_is_abs_path(include_file) )
    path = util_alloc_string_copy(include_file);
  else
  {
    util_alloc_file_components(current_file_realpath, &path, NULL, NULL);
    if( path == NULL )
      path = util_alloc_string_copy("/");
    path = util_strcat_realloc(path, "/");
    path = util_strcat_realloc(path, include_file);
  }

  realpath_ok = util_try_alloc_realpath(path);
  if( realpath_ok )
    *include_file_realpath = util_alloc_realpath(path);
  else
    *include_file_realpath = NULL;
                                                      
  free(path);

  if( realpath_ok )
    return CONF_OK;
  else
    return CONF_UNABLE_TO_OPEN_FILE;
}




/**
  Recursively builds a token buffer from files, and an associated
  stringlist of the source file for each token.
*/
static
int create_token_buffer__(
  stringlist_type     ** tokens,      /** Memory to the created pointer.      */
  stringlist_type     ** src_files,   /** Memory to the created pointer.      */
  const set_type       * sources,     /** Realpath of files already sourced.  */
  const tokenizer_type * tokenizer,   /** Tokenizer to use.                   */
  const char       * filename         /** Real path of file to tokenize.      */
)
{
  int status;

  stringlist_type * tokens__;
  stringlist_type * src_files__;
  

  /**
    Check if the file has been parsed before.
  */
  if( !set_has_key(sources, filename) )
    status = CONF_OK;
  else
  {
    /**
      TODO Add errror message.
    */
    status = CONF_RECURSIVE_INCLUDE_ERROR;
  }


  /**
    Check that we can open the file.  
  */
  if( status == CONF_OK && !util_fopen_test(filename, "r") )
  { 
    /**
      TODO Add errror message.
    */
    status = CONF_UNABLE_TO_OPEN_FILE;
  }


  if( status != CONF_OK )
  {
    /**
      Create empty tokens__ and src_files__ list to return.
    */
    tokens__    = stringlist_alloc_new();
    src_files__ = stringlist_alloc_new();
  }
  else
  {
    set_type * sources__   = set_copyc(sources);
    set_add_key(sources__, filename);

    tokens__    = tokenize_file(tokenizer, filename, true);
    src_files__ = stringlist_alloc_new(); 

    /**
      Set the sources. Let the first token own the memory.
    */
    for(int i=0; i<stringlist_get_size(tokens__); i++)
    {
      if(i == 0)
        stringlist_append_copy(src_files__, filename);
      else
        stringlist_append_ref(src_files__, stringlist_iget(src_files__, 0));
    }

    /**
      Handle any include statements by recursion.
    */
    {
      int i = stringlist_find_first(tokens__, "include");
      while( i != -1)
      {
        char * include_file;
        stringlist_type * included_tokens    = NULL;
        stringlist_type * included_src_files = NULL;

        /**
          Check that the next token actually exists.
        */
        if( i+1 == stringlist_get_size(tokens__) )
        {
          status = CONF_UNEXPECTED_EOF_ERROR;

          /**
           TODO Add error message.
           Delete the last tokens, so that the output is still meaningful.
          */
          stringlist_idel(tokens__, i);
          stringlist_idel(src_files__, i);
          break;
        }

        /**
          Check that the file exists.
        */
        status = alloc_included_realpath(filename, 
                                         stringlist_iget(tokens__, i+1),
                                         &include_file);
        if( status == CONF_OK )
        {
          /**
            Recursive call.
          */
          int last_status = create_token_buffer__(&included_tokens,
                                                 &included_src_files,
                                                 sources__, tokenizer, 
                                                 include_file);
          if( last_status != CONF_OK)
            status = last_status;

          free(include_file);
        }
        else
        {
          /**
            TODO Add error message.
          */
        }
          

        /**
          Delete the "include" and filename token. Do the same for src_files__.
        */
        stringlist_idel(tokens__, i+1);
        stringlist_idel(tokens__, i);
        stringlist_idel(src_files__, i+1);
        stringlist_idel(src_files__, i);

        if(included_tokens != NULL && included_src_files != NULL)
        {
          /**
            Replace the deleted items.
          */
          stringlist_insert_stringlist_copy(tokens__, included_tokens, i);
          stringlist_free(included_tokens);
          stringlist_insert_stringlist_copy(src_files__, included_src_files, i);
          stringlist_free(included_src_files);
        }
        

        /**
          Find next include statement.
        */
        i = stringlist_find_first(tokens__, "include");
      }
    }

    set_free(sources__);
  }

  *tokens    = tokens__;
  *src_files = src_files__;
  return status;
}



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
  char            * realpath_filename = util_alloc_realpath(filename);

  int status = create_token_buffer__(&tokens__, &src_files__, sources,
                                     tokenizer, realpath_filename);
  set_free(sources);
  free(realpath_filename);
  tokenizer_free(tokenizer);

  *tokens    = tokens__;
  *src_files = src_files__;

  return status;
}
