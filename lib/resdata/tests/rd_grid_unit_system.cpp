#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_file.hpp>

void test_EGRID(const char *filename, ert_rd_unit_enum unit_system) {
    rd_grid_ptr grid = make_rectangular_grid(4, 4, 2, 1, 1, 1, NULL);
    rd_grid_fwrite_EGRID2(grid.get(), filename, unit_system);

    rd_grid_ptr grid2 = read_grid(filename);
    test_assert_int_equal(rd_grid_get_unit_system(grid2.get()), unit_system);
}

void test_GRID(const char *filename, ert_rd_unit_enum unit_system) {
    rd_grid_ptr grid = make_rectangular_grid(4, 4, 2, 1, 1, 1, NULL);
    rd_grid_fwrite_GRID2(grid.get(), filename, unit_system);

    rd_grid_ptr grid2 = read_grid(filename);
    test_assert_int_equal(rd_grid_get_unit_system(grid2.get()), unit_system);
}

int main(int argc, char **argv) {
    rd::util::TestArea ta("grid_unit_system");

    test_EGRID("METRIC.EGRID", RD_METRIC_UNITS);
    test_EGRID("FIELD.EGRID", RD_FIELD_UNITS);
    test_GRID("METRIC.GRID", RD_METRIC_UNITS);
    test_GRID("FIELD.GRID", RD_FIELD_UNITS);
}
