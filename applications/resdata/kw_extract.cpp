#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <ert/util/util.hpp>
#include <ert/util/hash.hpp>
#include <ert/util/stringlist.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/fortio.h>
#include <resdata/rd_util.hpp>
#include <resdata/rd_sum.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_endian_flip.hpp>

/**
   This file will extract all occurences of kw1,kw2,...,kwn from the
   source file and copy them over to the target file. Ordering in the
   target file will be according to the ordering in the source file,
   and not by the ordering given on the command line.
*/

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "%s  src_file target_file kw1 kw2 kw3 \n", argv[0]);
        exit(0);
    }
    {
        const char *src_file = argv[1];
        const char *target_file = argv[2];
        fortio_type *fortio_src;
        fortio_type *fortio_target;
        bool fmt_src, fmt_target;
        stringlist_type *kw_set = stringlist_alloc_new();

        for (int iarg = 3; iarg < argc; iarg++)
            stringlist_append_copy(kw_set, argv[iarg]);

        if (!rd_fmt_file(src_file, &fmt_src))
            util_exit("Hmm - could not determine formatted/unformatted status "
                      "for:%s \n",
                      src_file);

        fmt_target = fmt_src; /* Can in principle be different */
        fortio_src = fortio_open_reader(src_file, fmt_src, RD_ENDIAN_FLIP);
        fortio_target =
            fortio_open_writer(target_file, fmt_target, RD_ENDIAN_FLIP);

        {
            rd_kw_type *rd_kw = rd_kw_alloc_empty();
            while (true) {
                if (rd_kw_fread_header(rd_kw, fortio_src) == RD_KW_READ_OK) {
                    const char *header = rd_kw_get_header(rd_kw);
                    if (stringlist_contains(kw_set, header)) {
                        rd_kw_fread_realloc_data(rd_kw, fortio_src);
                        rd_kw_fwrite(rd_kw, fortio_target);
                    } else
                        rd_kw_fskip_data(rd_kw, fortio_src);
                } else
                    break; /* We have reached EOF */
            }
            rd_kw_free(rd_kw);
        }

        fortio_fclose(fortio_src);
        fortio_fclose(fortio_target);
        stringlist_free(kw_set);
    }
}
