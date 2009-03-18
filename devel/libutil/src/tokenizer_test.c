#include <tokenizer.h>

int main(int argc, char ** argv)
{
  tokenizer_type * tokenizer = tokenizer_alloc(" \t\r\n", NULL, NULL, "##", "##");

  stringlist_type * tokens = tokenize_buffer( tokenizer, "svada bjarne spiser mye gress ## Kommentar som ikke skal syntes ## fordi han tror han er \n en ku.", true);

  int num_tokens = stringlist_get_size(tokens);

  for(int i = 0; i < num_tokens; i++)
    printf("token[%d] : %s\n", i, stringlist_iget(tokens, i) );

  stringlist_free( tokens );
  tokenizer_free( tokenizer );
  return 0;
}
