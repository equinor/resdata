#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.h>

#include <ert/ecl/ecl_util.hpp>

#include <ert/ecl_well/well_conn_collection.hpp>
#include <ert/ecl_well/well_conn.hpp>

void test_empty() {
    well_conn_collection_type *wellcc = well_conn_collection_alloc();
    test_assert_not_NULL(wellcc);
    test_assert_true(well_conn_collection_is_instance(wellcc));

    test_assert_int_equal(0, well_conn_collection_get_size(wellcc));
    {
        well_conn_type *conn = well_conn_collection_iget(wellcc, 0);
        test_assert_NULL(conn);
    }
    {
        const well_conn_type *conn =
            well_conn_collection_iget_const(wellcc, 10);
        test_assert_NULL(conn);
    }

    well_conn_collection_free(wellcc);
}

int main(int argc, char **argv) {
    test_empty();
    exit(0);
}
