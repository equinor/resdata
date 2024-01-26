#include <time.h>
#include <stdbool.h>
#include <signal.h>

#include <ert/util/util.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_util.hpp>

#include <resdata/well/well_state.hpp>
#include <resdata/well/well_info.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_ts.hpp>

int main(int argc, char **argv) {
    signal(SIGSEGV, util_abort_signal); /* Segmentation violation,
                                             i.e. overwriting memory ... */
    signal(SIGTERM, util_abort_signal); /* If killing the enkf program with
                                             SIGTERM (the default kill signal)
                                             you will get a backtrace.  Killing
                                             with SIGKILL (-9) will not give a
                                             backtrace.*/
    signal(SIGABRT, util_abort_signal); /* Signal abort. */
    {
        rd_grid_type *grid = rd_grid_alloc(argv[1]);
        well_info_type *well_info = well_info_alloc(grid);

        well_info_load_rstfile(well_info, argv[2], true);

        // List all wells:
        for (int iwell = 0; iwell < well_info_get_num_wells(well_info);
             iwell++) {
            well_ts_type *well_ts = well_info_get_ts(
                well_info, well_info_iget_well_name(well_info, iwell));
            well_state_type *well_state = well_ts_get_last_state(well_ts);
            test_assert_not_NULL(well_state);
        }
        well_info_free(well_info);
    }

    exit(0);
}
