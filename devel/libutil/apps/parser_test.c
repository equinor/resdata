#include <parser.h>

int main(int argc, char ** argv)
{
  parser_type * parser = parser_alloc(" \t\n\r,", "'\"", "[]{}=", NULL , "--", "\n");

  if(argc < 2 )
  {
    printf("Usage: parser_test.x file.txt\n");
    return 1;
  }

  stringlist_type * tokens = parser_tokenize_file(parser, argv[1] , true);

  int num_tokens = stringlist_get_size(tokens);

  for(int i = 0; i < num_tokens; i++)
    printf("token[%d] : %s\n", i, stringlist_iget(tokens, i) );
  
  stringlist_free( tokens );
  parser_free( parser );
  return 0;
}
