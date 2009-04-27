#include <assert.h>
#include <string.h>
#include <util.h>
#include <hash.h>
#include <set.h>
#include <vector.h>
#include <int_vector.h>
#include <conf.h>
#include <tokenizer.h>



/******************************************************************************/



#define CONF_INCLUDE_STRING "include"



#define CONF_SPEC_ID 132489012
#define CONF_ITEM_ID 342349032
#define CONF_ID      314234239



/******************************************************************************/



struct conf_spec_struct
{
  int                    __id;
  char                 * name;
  const conf_spec_type * super;
  hash_type            * specs;
  validator_ftype      * validator;
};



struct conf_struct
{
  int               __id; 
  conf_type       * super;
  char            * type;
  char            * name;
  vector_type     * confs;
  hash_type       * items;
};



struct conf_item_struct
{
  int               __id;
  stringlist_type * values;
};



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



/**
  Find the realpath of file included from another file. 
  Returns CONF_OK and stores the realpath in *include_file_realpath
  if the realpath can be resolved. If the realopath cannot be resolved,
  the function returns CONF_UNABLE_TO_OPEN_FILE and *include_file_realpath
  is set to NULL.

  The calling scope is responsible for free'ing *include_file_realpath if 
  CONF_OK is returned.
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
  assert(include_file_realpath != NULL);

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
  Recursively builds a stringlist of tokens from files, and an associated
  stringlist of the source file for each token.

  Do not use this function directly, use create_token_buffer.
*/
static
int create_token_buffer__(
  stringlist_type     ** tokens,      /** Memory to the created pointer.      */
  stringlist_type     ** src_files,   /** Memory to the created pointer.      */
  const set_type       * sources,     /** Real path of files already sourced. */
  const tokenizer_type * tokenizer,   /** Tokenizer to use.                   */
  const char           * filename ,   /** Real path of file to tokenize.      */
  stringlist_type      * errors       /** Storage for error messages.         */
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
    char * err = util_alloc_sprintf("Circular inclusion of \"%s\".", filename);
    stringlist_append_owned_ref(errors, err);
    status = CONF_CIRCULAR_INCLUDE_ERROR;
  }


  /**
    Check that we can open the file.  
  */
  if( status == CONF_OK && !util_fopen_test(filename, "r") )
  { 
    char * err = util_alloc_sprintf("Unable to open file \"%s\".", filename);
    stringlist_append_owned_ref(errors, err);
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
      int i = stringlist_find_first(tokens__, CONF_INCLUDE_STRING);
      while( i != -1)
      {
        const char * include_file;
        char       * include_file_realpath;
        stringlist_type * included_tokens    = NULL;
        stringlist_type * included_src_files = NULL;

        /**
          Check that the next token actually exists.
        */
        if( i+1 == stringlist_get_size(tokens__) )
        {
          char * err = util_alloc_sprintf("Unexpected end of file in \"%s\". "
                                          "Expected a file name after \"%s\".",
                                          filename, CONF_INCLUDE_STRING);
          stringlist_append_owned_ref(errors, err);
          status = CONF_UNEXPECTED_EOF_ERROR;

          /**
           Delete the last tokens, so that the output is still meaningful.
          */
          stringlist_idel(tokens__, i);
          stringlist_idel(src_files__, i);
          break;
        }

        include_file = stringlist_iget(tokens__, i+1);

        /**
          Check that the file exists.
        */
        status = alloc_included_realpath(filename, 
                                         include_file,
                                         &include_file_realpath);
        if( status == CONF_OK )
        {
          /**
            Recursive call.
          */
          int last_status = create_token_buffer__(&included_tokens,
                                                 &included_src_files,
                                                 sources__, tokenizer, 
                                                 include_file_realpath,
                                                 errors);
          if( last_status != CONF_OK)
            status = last_status;

          free(include_file_realpath);
        }
        else
        {
          char * err = util_alloc_sprintf("Unable to open file \"%s\" included "
                                          "from file \"%s\".", 
                                          include_file, filename);
          stringlist_append_owned_ref(errors, err);
          status = CONF_UNABLE_TO_OPEN_FILE;
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
        i = stringlist_find_first(tokens__, CONF_INCLUDE_STRING);
      }
    }

    set_free(sources__);
  }

  *tokens    = tokens__;
  *src_files = src_files__;
  return status;
}



/**
  Creates a stringlist of tokens suitable for parsing with the config
  parser from a file. All include statements are resolved, and circular
  include statements are detected.

  If no errors are detected CONF_OK will be returned. Otherwise, the last
  detected error will be returned. Error messages are available in *errors.
  
  Note that the recursion does not halt on errors. Thus, the tokens are fully
  useable even though errors may have been encountered.

  The calling scope is responsible for free'ing *tokens, *src_files and 
  *errors when they are no longer needed.
*/
static
int create_token_buffer(
  stringlist_type ** tokens,    /** Memory to return the allocated tokens in. */
  stringlist_type ** src_files, /** Memory to return source files in.         */
  stringlist_type ** errors,    /** Memory to return error messages in.       */
  const char       * filename   /** Filename to create tokens from.           */
)
{
  int status;
  stringlist_type * tokens__   ; 
  stringlist_type * src_files__;

  stringlist_type * errors__ = stringlist_alloc_new(); 

  if( util_fopen_test(filename, "r") )
  {
    tokenizer_type * tokenizer   = tokenizer_alloc(" \t\n\r,;", "'\"", "[]{}",
                                                 "--", "\n");
    set_type * sources           = set_alloc_empty();
    char     * realpath_filename = util_alloc_realpath(filename);

    status = create_token_buffer__(&tokens__, &src_files__, sources,
                                   tokenizer, realpath_filename, errors__);

    free(realpath_filename);
    set_free(sources);
    tokenizer_free(tokenizer);
  }
  else
  {
    tokens__    = stringlist_alloc_new();
    src_files__ = stringlist_alloc_new();

    char * err = util_alloc_sprintf("Unable to open file \"%s\".", filename);
    stringlist_append_owned_ref(errors__, err);

    status      = CONF_UNABLE_TO_OPEN_FILE;
  }

  *tokens    = tokens__;
  *src_files = src_files__;
  *errors    = errors__;

  return status;
}



/******************************************************************************/



static
conf_type * conf_alloc(
  conf_type       * super,
  const char      * type,
  const char      * name
)
{
  assert(type != NULL);
  assert(name != NULL);

  conf_type * conf = util_malloc(sizeof * conf, __func__);

  conf->__id  = CONF_ID;
  conf->super = super;
  conf->type  = util_alloc_string_copy(type);
  conf->name  = util_alloc_string_copy(name);

  conf->confs = vector_alloc_new();
  conf->items = hash_alloc();

  return conf;
}



static
conf_type * conf_safe_cast(
  void * conf
)
{
  conf_type * __conf = (conf_type *) conf;
  if( __conf->__id != CONF_ID )
    util_abort("%s: Internal error. Run time cast failed.\n", __func__);
  return __conf;
}



void conf_free(
  conf_type * conf
)
{
  hash_free(conf->items);
  vector_free(conf->confs);

  free(conf->type);
  free(conf->name);
  free(conf);
}



static
void conf_free__(
  void * conf
)
{
  conf_type * conf__ = conf_safe_cast(conf);
  conf_free(conf__);
}



static
conf_item_type * conf_item_alloc()
{
  conf_item_type * item = util_malloc(sizeof * item, __func__);
  item->__id   = CONF_ITEM_ID;
  item->values = stringlist_alloc_new();
}

 

static
conf_item_type * conf_item_safe_cast(
  void * item
)
{
  conf_item_type * __item = (conf_item_type *) item;
  if( __item->__id != CONF_ITEM_ID)
    util_abort("%s: Internal error. Run time cast failed.\n", __func__);
  return __item;
}



static
void conf_item_free(
  conf_item_type * item
)
{
  stringlist_free(item->values);
  free(item);
}



static
void conf_item_free__(
  void * item
)
{
  conf_item_type * item__ = conf_item_safe_cast(item);
  conf_item_free(item__);
}



/******************************************************************************/



static
void conf_insert_item(
  conf_type      * conf,
  const char     * name,
  conf_item_type * item
)
{
  assert(conf != NULL);
  assert(name != NULL);
  assert(item != NULL);

  hash_insert_hash_owned_ref(conf->items, name, item, conf_item_free__);
}



static
void conf_append_child(
  conf_type * conf,
  conf_type * conf_child
)
{
  assert(conf       != NULL);
  assert(conf_child != NULL);

  vector_append_owned_ref(conf->confs, conf_child, conf_free__);
}



static
conf_type * conf_get_super(
  conf_type * conf
)
{
  return conf->super;
}
