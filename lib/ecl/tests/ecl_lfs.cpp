#include <stdlib.h>
#include <stdbool.h>

#include <ert/ecl/ecl_kw.hpp>
#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_endian_flip.hpp>
#include <ert/ecl/ecl_type.hpp>

int main(int argc, char **argv) {
    int num_kw = 1000; // Total file size should roughly exceed 2GB
    int kw_size = 600000;
    ecl_kw_type *kw = ecl_kw_alloc("KW", kw_size, ECL_INT);
    int i;
    offset_type file_size;
    for (i = 0; i < kw_size; i++)
        ecl_kw_iset_int(kw, i, i);

    {
        fortio_type *fortio =
            fortio_open_writer("LARGE_FILE.UNRST", false, ECL_ENDIAN_FLIP);
        for (i = 0; i < num_kw; i++) {
            printf("Writing keyword %d/%d to file:LARGE_FILE.UNRST \n", i + 1,
                   num_kw);
            ecl_kw_fwrite(kw, fortio);
        }
        fortio_fclose(fortio);
    }

    /*{
    fortio_type * fortio = fortio_open_reader( "LARGE_FILE.UNRST" , false , ECL_ENDIAN_FLIP);
    for (i = 0; i < num_kw - 1; i++) {
       printf("SKipping keyword %d/%d from file:LARGE_FILE.UNRST \n",i+1 , num_kw );
       ecl_kw_fskip( fortio );
    }
    {
       ecl_kw_type * file_kw = ecl_kw_fread_alloc( fortio );
       if (ecl_kw_equal( kw , file_kw ))
          printf("Keyword read back from file correctly :-) \n");
        else
          printf("Fatal error - keyword different on return ...\n");
       ecl_kw_free( file_kw );
    }
    fortio_fclose( fortio );
  }
  */
    file_size = util_file_size("LARGE_FILE.UNRST");
    printf("File size: %lld \n", file_size);
    {
        fortio_type *fortio =
            fortio_open_reader("LARGE_FILE.UNRST", false, ECL_ENDIAN_FLIP);
        printf("Seeking to file end: ");
        fortio_fseek(fortio, file_size, SEEK_SET);
        fortio_fclose(fortio);
        printf("Seek OK \n");
    }

    printf("Doing ecl_file_open(..)\n");
    {
        ecl_file_type *file = ecl_file_open("LARGE_FILE.UNRST", 0);
        ecl_kw_type *file_kw = ecl_file_iget_named_kw(file, "KW", num_kw - 1);
        if (ecl_kw_equal(kw, file_kw))
            printf("Keyword read back from file correctly :-) \n");
        else
            printf("Fatal error - keyword different on return ...\n");
        ecl_file_close(file);
    }

    remove("LARGE_FILE.UNRST");

    exit(0);
}
