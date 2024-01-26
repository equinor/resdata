#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>

#include <resdata/well/well_state.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();

    test_assert_int_equal(
        well_state_translate_rd_type_int(IWEL_UNDOCUMENTED_ZERO), RD_WELL_ZERO);
    test_assert_int_equal(well_state_translate_rd_type_int(IWEL_PRODUCER),
                          RD_WELL_PRODUCER);
    test_assert_int_equal(well_state_translate_rd_type_int(IWEL_WATER_INJECTOR),
                          RD_WELL_WATER_INJECTOR);
    test_assert_int_equal(well_state_translate_rd_type_int(IWEL_GAS_INJECTOR),
                          RD_WELL_GAS_INJECTOR);
    test_assert_int_equal(well_state_translate_rd_type_int(IWEL_OIL_INJECTOR),
                          RD_WELL_OIL_INJECTOR);

    {
        const char *well_name = "WELL";
        int report_nr = 100;
        int global_well_nr = 67;
        time_t valid_from = -1;
        bool open = false;
        well_type_enum type = RD_WELL_GAS_INJECTOR;

        well_state_type *well_state = well_state_alloc(
            well_name, global_well_nr, open, type, report_nr, valid_from);
        test_assert_true(well_state_is_instance(well_state));

        test_assert_false(well_state_is_MSW(well_state));
        test_assert_string_equal(well_name, well_state_get_name(well_state));
        test_assert_int_equal(global_well_nr,
                              well_state_get_well_nr(well_state));
        test_assert_bool_equal(open, well_state_is_open(well_state));
        test_assert_int_equal(type, well_state_get_type(well_state));
        test_assert_int_equal(report_nr, well_state_get_report_nr(well_state));
        test_assert_time_t_equal(valid_from,
                                 well_state_get_sim_time(well_state));

        test_assert_NULL(well_state_get_global_connections(well_state));
        test_assert_false(well_state_has_global_connections(well_state));
        test_assert_NULL(well_state_get_grid_connections(well_state, "GRID"));
        test_assert_false(well_state_has_grid_connections(well_state, "GRID"));

        test_assert_double_equal(0.0, well_state_get_oil_rate(well_state));
        test_assert_double_equal(0.0, well_state_get_gas_rate(well_state));
        test_assert_double_equal(0.0, well_state_get_water_rate(well_state));
        test_assert_double_equal(0.0, well_state_get_volume_rate(well_state));

        well_state_free(well_state);
    }
    exit(0);
}
