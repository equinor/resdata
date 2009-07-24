#include <tokenizer.h>

int main(int argc, char ** argv)
{
  tokenizer_type * tokenizer = tokenizer_alloc(" \t\n\r,", "'\"", "[]{}=", NULL , "--", "\n");

  if(argc < 2 )
  {
    printf("Usage: tokenizer_test.x file.txt\n");
    return 1;
  }

  stringlist_type * tokens = tokenize_file(tokenizer, argv[1] , true);

  int num_tokens = stringlist_get_size(tokens);

  for(int i = 0; i < num_tokens; i++)
    printf("token[%d] : %s\n", i, stringlist_iget(tokens, i) );

  stringlist_free( tokens );
  tokenizer_free( tokenizer );
  return 0;
}
