#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_kw.hpp>

int main(int argc, char **argv) {
    int i;
    rd_kw_type *rd_kw = rd_kw_alloc("HEAD", 10, RD_INT);

    for (i = 0; i < 10; i++)
        rd_kw_iset_int(rd_kw, i, i);

    {
        rd::util::TestArea ta("kw_grdecl");
        FILE *stream = util_fopen("FILE.grdecl", "w");

        rd_kw_fprintf_grdecl(rd_kw, stream);
        fclose(stream);

        stream = util_fopen("FILE.grdecl", "r");
        {
            rd_kw_type *rd_kw2 =
                rd_kw_fscanf_alloc_grdecl(stream, "HEAD", 10, RD_INT);

            test_assert_not_NULL(rd_kw2);
            test_assert_true(rd_kw_equal(rd_kw, rd_kw2));
            rd_kw_free(rd_kw2);
        }
        fclose(stream);

        stream = util_fopen("FILE.grdecl", "w");
        rd_kw_fprintf_grdecl__(rd_kw, "HEAD1234", stream);
        fclose(stream);

        stream = util_fopen("FILE.grdecl", "r");
        {
            rd_kw_type *rd_kw2 =
                rd_kw_fscanf_alloc_grdecl(stream, "HEAD", 10, RD_INT);

            test_assert_NULL(rd_kw2);
            rd_kw2 = rd_kw_fscanf_alloc_grdecl(stream, "HEAD1234", 10, RD_INT);
            test_assert_not_NULL(rd_kw2);

            test_assert_string_equal(rd_kw_get_header(rd_kw2), "HEAD1234");
            test_assert_true(rd_kw_content_equal(rd_kw, rd_kw2));
            rd_kw_free(rd_kw2);
        }
        fclose(stream);
    }
    rd_kw_free(rd_kw);

    exit(0);
}
