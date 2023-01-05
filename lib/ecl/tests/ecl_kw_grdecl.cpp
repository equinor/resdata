#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>
#include <ert/util/test_work_area.hpp>

#include <ert/ecl/ecl_kw.hpp>

int main(int argc, char **argv) {
    int i;
    ecl_kw_type *ecl_kw = ecl_kw_alloc("HEAD", 10, ECL_INT);

    for (i = 0; i < 10; i++)
        ecl_kw_iset_int(ecl_kw, i, i);

    {
        ecl::util::TestArea ta("kw_grdecl");
        FILE *stream = util_fopen("FILE.grdecl", "w");

        ecl_kw_fprintf_grdecl(ecl_kw, stream);
        fclose(stream);

        stream = util_fopen("FILE.grdecl", "r");
        {
            ecl_kw_type *ecl_kw2 =
                ecl_kw_fscanf_alloc_grdecl(stream, "HEAD", 10, ECL_INT);

            test_assert_not_NULL(ecl_kw2);
            test_assert_true(ecl_kw_equal(ecl_kw, ecl_kw2));
            ecl_kw_free(ecl_kw2);
        }
        fclose(stream);

        stream = util_fopen("FILE.grdecl", "w");
        ecl_kw_fprintf_grdecl__(ecl_kw, "HEAD1234", stream);
        fclose(stream);

        stream = util_fopen("FILE.grdecl", "r");
        {
            ecl_kw_type *ecl_kw2 =
                ecl_kw_fscanf_alloc_grdecl(stream, "HEAD", 10, ECL_INT);

            test_assert_NULL(ecl_kw2);
            ecl_kw2 =
                ecl_kw_fscanf_alloc_grdecl(stream, "HEAD1234", 10, ECL_INT);
            test_assert_not_NULL(ecl_kw2);

            test_assert_string_equal(ecl_kw_get_header(ecl_kw2), "HEAD1234");
            test_assert_true(ecl_kw_content_equal(ecl_kw, ecl_kw2));
            ecl_kw_free(ecl_kw2);
        }
        fclose(stream);
    }
    ecl_kw_free(ecl_kw);

    exit(0);
}
