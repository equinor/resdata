#include <cstdio>
#include <cstdlib>
#include <cstdio>

#include <ios>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/FortIO.hpp>

#include <ert/util/util.hpp>

int main(int argc, char **argv) {
    int num_kw = 1000; // Total file size should roughly exceed 2GB
    int kw_size = 600000;
    rd_kw_type *kw = rd_kw_alloc("KW", kw_size, RD_INT);
    int i;
    offset_type file_size;
    for (i = 0; i < kw_size; i++)
        rd_kw_iset_int(kw, i, i);

    {
        ERT::FortIO fortio("LARGE_FILE.UNRST", std::ios_base::out);
        for (i = 0; i < num_kw; i++) {
            printf("Writing keyword %d/%d to file:LARGE_FILE.UNRST \n", i + 1,
                   num_kw);
            rd_kw_fwrite(kw, fortio.get());
        }
    }

    file_size = util_file_size("LARGE_FILE.UNRST");
    printf("File size: %lld \n", file_size);
    {
        ERT::FortIO fortio("LARGE_FILE.UNRST", std::ios_base::in);
        printf("Seeking to file end: ");
        fortio_fseek(fortio.get(), file_size, SEEK_SET);
        printf("Seek OK \n");
    }

    printf("Doing rd_file_open(..)\n");
    {
        rd_file_type *file = rd_file_open("LARGE_FILE.UNRST", 0);
        rd_kw_type *file_kw = rd_file_iget_named_kw(file, "KW", num_kw - 1);
        if (rd_kw_equal(kw, file_kw))
            printf("Keyword read back from file correctly :-) \n");
        else
            printf("Fatal error - keyword different on return ...\n");
        rd_file_close(file);
    }

    remove("LARGE_FILE.UNRST");

    exit(0);
}
