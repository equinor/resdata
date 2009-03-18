#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__
#include <stringlist.h>

typedef struct tokenizer_struct tokenizer_type;


tokenizer_type * tokenizer_alloc(
  const char * whitespace,       /** Need to contain at least one non '\0' char. */
  const char * quoters,          /** Set to NULL if not interessting.            */
  const char * specials,         /** Set to NULL if not interessting.            */
  const char * comment_start,    /** Set to NULL if not interessting.            */
  const char * comment_end);     /** Set to NULL  if not interessting.            */

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
