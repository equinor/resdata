#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_kw.hpp>

int main(int argc, char **argv) {
    rd::util::TestArea ta("grid_unit_system");

    // 1. Write a rd_kw instance with string data - uninitialized.
    {
        rd_kw_type *rd_kw = rd_kw_alloc("SPACE", 1, RD_CHAR);
        fortio_type *f = fortio_open_writer("file", false, true);
        rd_kw_fwrite(rd_kw, f);
        fortio_fclose(f);
        rd_kw_free(rd_kw);
    }

    // 2. Open file with normal fopen() and verify that the data section consists of only spaces.
    {
        FILE *stream = util_fopen("file", "r");
        char buffer[8];
        size_t offset = 4 + 16 + 4 + 4;
        fseek(stream, offset, SEEK_SET);
        fread(buffer, 1, 8, stream);
        for (int i = 0; i < 8; i++)
            test_assert_int_equal(buffer[i], ' ');

        fclose(stream);
    }
}
