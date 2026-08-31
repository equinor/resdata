#include "ert/util/util.hpp"
#include <cstdio>
#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <resdata/rd_grid.hpp>
#include <vector>

typedef struct {
    double x;
    double y;
    double z;

    int g;
    int i;
    int j;
    int k;

    bool skip;
} point_type;

std::vector<point_type> load_expected(const rd_grid_type *grid,
                                      const char *filename) {
    FILE *stream = util_fopen(filename, "r");
    std::vector<point_type> expected;

    while (true) {
        double x, y, z;
        int i, j, k, skip;

        if (fscanf(stream, "%lg %lg %lg %d %d %d %d", &x, &y, &z, &i, &j, &k,
                   &skip) == 7) {
            point_type p;
            p.x = x;
            p.y = y;
            p.z = z;

            p.i = i - 1;
            p.j = j - 1;
            p.k = k - 1;
            p.skip = skip;
            p.g = rd_grid_get_global_index3(grid, p.i, p.j, p.k);
            expected.push_back(p);
        } else
            break;
    }

    fclose(stream);
    test_assert_size_t_equal(10, expected.size());
    return expected;
}

void test_well_point(rd_grid_type *grid, const point_type *expected) {
    int g = rd_grid_get_global_index_from_xyz(grid, expected->x, expected->y,
                                              expected->z, 0);
    if (g != rd_grid_get_global_index3(grid, expected->i, expected->j,
                                       expected->k)) {
        int i, j, k;
        rd_grid_get_ijk1(grid, g, &i, &j, &k);
        bool g_contains_xyz = rd_grid_cell_contains_xyz1(
            grid, expected->g, expected->x, expected->y, expected->z);
        fprintf(stderr,
                "(%g,%g,%g) differs:  Grid: %d:(%d,%d,%d)   Expected: "
                "%d:(%d,%d,%d)  contains:%d\n",
                expected->x, expected->y, expected->z, g, i, j, k, expected->g,
                expected->i, expected->j, expected->k, g_contains_xyz);
    }
    if (!expected->skip)
        test_assert_int_equal(g, rd_grid_get_global_index3(grid, expected->i,
                                                           expected->j,
                                                           expected->k));
    else {
        if (g != rd_grid_get_global_index3(grid, expected->i, expected->j,
                                           expected->k))
            fprintf(stderr, " ** Skipping failed test for point: %g %g %g \n",
                    expected->x, expected->y, expected->z);
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    {
        rd_grid_ptr grid = read_grid(argv[1]);
        auto expected = load_expected(grid.get(), argv[2]);

        for (auto &p : expected) {
            test_well_point(grid.get(), &p);
        }
    }
    exit(0);
}
