#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__
#include <stringlist.h>

typedef struct tokenizer_struct tokenizer_type;


/**
  GENERAL OVERVIEW

  The tokenizer_type is used to create a series of "tokens"
  from a file or string buffer. In it's simplest form,
  we define a token as a subset of a string separated by
  by some split characters.

  For example, if we define the normal space (i.e. " ") as
  the only split character, "tokenizing" the string
  "I like     beer  " would give the following result:

  Token number 0 is "I"
  Token number 1 is "like"
  Token number 2 is "beer"

  Note that all the white space (i.e. split characters) have been
  removed.



  COMMENTS

  The tokenizer can ignore comments when tokenzing
  a file or buffer. To enable this feature, allocate
  the tokenizer_type with comment_start and comment_end
  different from NULL. For example if we set both
  comment_start and comment_end to "##", tokenizing
  "I ## really  ## like beer" would give:

  Token number 0 is "I"
  Token number 1 is "like"
  Token number 2 is "beer"



  SPECIAL CHARACTERS 
  
  Some times it can be useful to define a set of characters which
  behave like white space in the sense that they separate tokens in
  the source, but they do not get dropped. For example, letting "=" be
  a special character, tokenzing "key=value" would give:

  Token number 0 is "key"
  Token number 1 is "="
  Token number 2 is "value"

  The special characters are given in the "specials" string when
  allocating the tokenizer.



  QUOTERS

  When parsing user input, the user often wants to provide e.g. a
  filename with a white-space character in it. To support this, the
  tokenizer can be given a set of quoters. For example, letting " " be
  white space and adding "'" to the quoters, tokenizing 

   "my_file = 'my documents with space in.txt'" 

  would give:

  Token number 0 is "my_file"
  Token number 1 is "="
  Token number 2 is "'my documents with space in.txt'"

  If wanted, the quoting characters can be removed
  using the strip_quote_marks options when running
  the tokenizer on the buffer. The last token
  in the example above would then be:

  Token number 2 is "my documents with space in.txt"

  To use one of the quoter characters in a string,
  place a "\" in front of it. Building on our previous
  example, let the string be "my_file = 'my \'doc.txt'"
  Tokenzing this with strip_quote_marks set to true
  would give:

  Token number 0 is "my_file"
  Token number 1 is "="
  Token number 2 is "my 'doc.txt"

  Note that the "\" in front of"'" has been removed.
  If strip_quote_marks is set to false, the result is:
  

  Token number 0 is "my_file"
  Token number 1 is "="
  Token number 2 is "'my \'doc.txt'"

*/


tokenizer_type * tokenizer_alloc(
  const char * whitespace,       /** Set to NULL if not interessting.         */
  const char * quoters,          /** Set to NULL if not interessting.         */
  const char * specials,         /** Set to NULL if not interessting.         */
  const char * delete_set, 
  const char * comment_start,    /** Set to NULL if not interessting.         */
  const char * comment_end);     /** Set to NULL if not interessting.         */

void tokenizer_free(
  tokenizer_type * tokenizer);


stringlist_type * tokenize_buffer(
  const tokenizer_type * tokenizer,
  const char           * buffer,
  bool                   strip_quote_marks);


stringlist_type * tokenize_file(
  const tokenizer_type * tokenizer,
  const char           * filename,
  bool                   strip_quote_marks);


#endif
