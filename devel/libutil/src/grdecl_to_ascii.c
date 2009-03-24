#include <assert.h>
#include <tokenizer.h>
#include <util.h>


int main(int argc, char ** argv)
{
  if(argc < 2)
    util_exit("usage: grdecl_to_ascii file_1.grdecl file_2.grdecl ... file_N.grdecl.\n");

  tokenizer_type * tokenizer = tokenizer_alloc(" \t\r\n", NULL , NULL , "--", "\n");

  for(int i=1; i<argc; i++)
  {
    char * basename;
    char * filename;
    FILE * stream;

    stringlist_type * tokens = tokenize_file(tokenizer, argv[i], false);
    int num_tokens = stringlist_get_size(tokens);

    util_alloc_file_components( argv[i], NULL, &basename, NULL);
    assert(basename != NULL);
    filename = util_alloc_filename(NULL, basename, "ascii");
    stream = util_fopen(filename, "w");

    for(int i=0; i<num_tokens; i++)
    {
      double value;
      const char * value_str = stringlist_iget(tokens, i);
      if( util_sscanf_double(value_str, &value) )
        fprintf(stream, "%f\n", value); 
    }

    fclose(stream);
    free(filename);
    free(basename);
  }

  tokenizer_free(tokenizer);
}
