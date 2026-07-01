#include <cstdlib>
#include <ctime>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/well/well_state.hpp>
#include <resdata/well/well_const.hpp>

int main(int argc, char **argv) {
    test_install_SIGNALS();

    test_assert_true(well_state_translate_rd_type_int(IWEL_UNDOCUMENTED_ZERO) ==
                     WellType::ZERO);
    test_assert_true(well_state_translate_rd_type_int(IWEL_PRODUCER) ==
                     WellType::PRODUCER);
    test_assert_true(well_state_translate_rd_type_int(IWEL_WATER_INJECTOR) ==
                     WellType::WATER_INJECTOR);
    test_assert_true(well_state_translate_rd_type_int(IWEL_GAS_INJECTOR) ==
                     WellType::GAS_INJECTOR);
    test_assert_true(well_state_translate_rd_type_int(IWEL_OIL_INJECTOR) ==
                     WellType::OIL_INJECTOR);

    {
        const char *well_name = "WELL";
        int report_nr = 100;
        int global_well_nr = 67;
        time_t valid_from = -1;
        bool open = false;
        auto type = WellType::GAS_INJECTOR;

        WellState well_state(well_name, global_well_nr, open, type, report_nr,
                             valid_from);

        test_assert_false(well_state.is_MSW());
        test_assert_string_equal(well_name, well_state.get_name().c_str());
        test_assert_int_equal(global_well_nr, well_state.get_well_nr());
        test_assert_bool_equal(open, well_state.is_open());
        test_assert_true(type == well_state.get_type());
        test_assert_int_equal(report_nr, well_state.get_report_nr());
        test_assert_time_t_equal(valid_from, well_state.get_sim_time());

        test_assert_NULL(well_state.get_global_connections());
        test_assert_false(well_state.has_global_connections());
        test_assert_NULL(well_state.get_grid_connections("GRID"));
        test_assert_false(well_state.has_grid_connections("GRID"));

        test_assert_double_equal(0.0, well_state.get_oil_rate());
        test_assert_double_equal(0.0, well_state.get_gas_rate());
        test_assert_double_equal(0.0, well_state.get_water_rate());
        test_assert_double_equal(0.0, well_state.get_volume_rate());
    }
    exit(0);
}
