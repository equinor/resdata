#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <ert/util/test_util.hpp>
#include <resdata/rd_grid.hpp>

bool get_test_point1(const rd_grid_type *grid, int global_index, double *_xpos,
                     double *_ypos, double *_zpos) {
    const int corners[4] = {1, 2, 5, 6};
    double xpos = 0;
    double ypos = 0;
    double zpos = 0;
    const double min_volume = 1e-8;

    if (fabs(rd_grid_get_cell_volume1(grid, global_index)) <= min_volume)
        return false;

    if (rd_grid_get_cell_twist1(grid, global_index) > 0)
        return false;

    if (!rd_grid_cell_regular1(grid, global_index))
        return false;

    for (int ci = 0; ci < 4; ci++) {
        int corner = corners[ci];
        double x, y, z;
        rd_grid_get_cell_corner_xyz1(grid, global_index, corner, &x, &y, &z);
        xpos += x;
        ypos += y;
        zpos += z;
    }

    *_xpos = xpos * 0.25;
    *_ypos = ypos * 0.25;
    *_zpos = zpos * 0.25;

    return rd_grid_cell_contains_xyz1(grid, global_index, *_xpos, *_ypos,
                                      *_zpos);
}

bool get_test_point3(const rd_grid_type *grid, int i, int j, int k,
                     double *_xpos, double *_ypos, double *_zpos) {
    const int global_index = rd_grid_get_global_index3(grid, i, j, k);
    return get_test_point1(grid, global_index, _xpos, _ypos, _zpos);
}

void test_grid_covering(const rd_grid_type *grid) {
    const int nx = rd_grid_get_nx(grid);
    const int ny = rd_grid_get_ny(grid);
    const int nz = rd_grid_get_nz(grid);

    for (int k = 0; k < nz - 1; k++) {
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                int g1 = rd_grid_get_global_index3(grid, i, j, k);
                int g2 = rd_grid_get_global_index3(grid, i, j, k + 1);
                double p1[3];
                double p2[3];

                for (int l = 0; l < 4; l++) {
                    rd_grid_get_cell_corner_xyz1(grid, g1, l + 4, &p1[0],
                                                 &p1[1], &p1[2]);
                    rd_grid_get_cell_corner_xyz1(grid, g2, l, &p2[0], &p2[1],
                                                 &p2[2]);
                    test_assert_true(p1[0] == p2[0]);
                    test_assert_true(p1[1] == p2[1]);
                    test_assert_true(p1[2] == p2[2]);
                }

                for (int l = 0; l < 4; l++) {
                    rd_grid_get_cell_corner_xyz1(grid, g1, l, &p1[0], &p1[1],
                                                 &p1[2]);
                    rd_grid_get_cell_corner_xyz1(grid, g1, l + 4, &p2[0],
                                                 &p2[1], &p2[2]);

                    test_assert_true(p2[2] >= p1[2]);
                }
            }
        }
    }
}

void assert_contains(const rd_grid_type *grid, int i, int j, int k, double x,
                     double y, double z) {
    if (!rd_grid_cell_contains_xyz3(grid, i, j, k, x, y, z))
        fprintf(stderr, " Point: (%g,%g,%g) not found in cell: (%d,%d,%d) \n",
                x, y, z, i, j, k);

    test_assert_true(rd_grid_cell_contains_xyz3(grid, i, j, k, x, y, z));
}

void test_contains(const rd_grid_type *grid) {
    const int nx = rd_grid_get_nx(grid);
    const int ny = rd_grid_get_ny(grid);
    const int nz = rd_grid_get_nz(grid);

    for (int k = 0; k < nz; k++) {
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                if (rd_grid_get_cell_twist3(grid, i, j, k) != 0) {
                    double x, y, z;
                    if (get_test_point3(grid, i, j, k, &x, &y, &z)) {
                        assert_contains(grid, i, j, k, x, y, z);
                        {
                            int i2, j2, k2;
                            int kmin = util_int_max(0, k - 1);
                            int kmax = util_int_min(nz, k + 1);

                            int jmin = util_int_max(0, j - 1);
                            int jmax = util_int_min(ny, j + 1);

                            int imin = util_int_max(0, i - 1);
                            int imax = util_int_min(nx, i + 1);

                            for (k2 = kmin; k2 < kmax; k2++) {
                                for (j2 = jmin; j2 < jmax; j2++) {
                                    for (i2 = imin; i2 < imax; i2++) {
                                        if ((i != i2) && (j != j2) &&
                                            (k != k2)) {
                                            if (rd_grid_get_cell_twist3(
                                                    grid, i2, j2, k2) == 0)
                                                test_assert_false(
                                                    rd_grid_cell_contains_xyz3(
                                                        grid, i2, j2, k2, x, y,
                                                        z));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void test_find(rd_grid_type *grid) {
    int init_index;
    int find_count = 100;
    int delta = util_int_max(1, rd_grid_get_global_size(grid) / find_count);
    for (init_index = 0; init_index < rd_grid_get_global_size(grid);
         init_index += delta) {
        if (rd_grid_get_cell_twist1(grid, init_index) == 0) {
            double x, y, z;
            int find_index;
            int start_index = 0;

            if (get_test_point1(grid, init_index, &x, &y, &z)) {
                find_index = rd_grid_get_global_index_from_xyz(grid, x, y, z,
                                                               start_index);
                if (init_index != find_index) {
                    int i1, j1, k1, i2, j2, k2;

                    rd_grid_get_ijk1(grid, init_index, &i1, &j1, &k1);
                    rd_grid_get_ijk1(grid, find_index, &i2, &j2, &k2);
                    printf("       point: %14.7f  %14.7f  %14.7f \n", x, y, z);
                    printf("       Regular: %d / %d \n",
                           rd_grid_cell_regular3(grid, i1, j1, k1),
                           rd_grid_cell_regular3(grid, i2, j2, k2));
                    printf(
                        "       init: contains(%7d) : %d   (%d,%d,%d) V:%g \n",
                        init_index,
                        rd_grid_cell_contains_xyz1(grid, init_index, x, y, z),
                        i1 + 1, j1 + 1, k1 + 1,
                        rd_grid_get_cell_volume1(grid, init_index));
                    printf(
                        "ERROR: find: contains(%7d) : %d   (%d,%d,%d) V:%g  "
                        "\n\n",
                        find_index,
                        rd_grid_cell_contains_xyz1(grid, find_index, x, y, z),
                        i2 + 1, j2 + 1, k2 + 1,
                        rd_grid_get_cell_volume1(grid, find_index));
                    test_assert_int_equal(init_index, find_index);
                }
            }
        }
    }
}

void test_corners() {
    rd_grid_type *grid = rd_grid_alloc_rectangular(3, 3, 3, 1, 1, 1, NULL);

    test_assert_int_equal(
        -1, rd_grid_get_global_index_from_xyz(grid, -1, -1, -1, 0));
    test_assert_int_equal(-1,
                          rd_grid_get_global_index_from_xyz(grid, -1, 1, 1, 0));
    test_assert_int_equal(-1,
                          rd_grid_get_global_index_from_xyz(grid, 1, -1, 1, 0));
    test_assert_int_equal(-1,
                          rd_grid_get_global_index_from_xyz(grid, 1, 1, -1, 0));

    test_assert_int_equal(
        -1, rd_grid_get_global_index_from_xyz(grid, 3.5, 3.5, 3.5, 0));
    test_assert_int_equal(
        -1, rd_grid_get_global_index_from_xyz(grid, 3.5, 1, 1, 0));
    test_assert_int_equal(
        -1, rd_grid_get_global_index_from_xyz(grid, 1, 3.5, 1, 0));
    test_assert_int_equal(
        -1, rd_grid_get_global_index_from_xyz(grid, 1, 1, 3.5, 0));

    {
        double x, y, z;
        int i;
        rd_grid_get_cell_corner_xyz3(grid, 0, 0, 0, 0, &x, &y, &z);
        test_assert_int_equal(
            0, rd_grid_get_global_index_from_xyz(grid, x, y, z, 0));

        for (i = 1; i < 8; i++) {
            rd_grid_get_cell_corner_xyz3(grid, 0, 0, 0, i, &x, &y, &z);
            test_assert_int_not_equal(
                0, rd_grid_get_global_index_from_xyz(grid, x, y, z, 0));
        }

        // Corner 1
        rd_grid_get_cell_corner_xyz3(grid, 2, 0, 0, 1, &x, &y, &z);
        test_assert_int_equal(
            rd_grid_get_global_index3(grid, 2, 0, 0),
            rd_grid_get_global_index_from_xyz(grid, x, y, z, 0));

        // Corner 2
        rd_grid_get_cell_corner_xyz3(grid, 0, 2, 0, 2, &x, &y, &z);
        test_assert_int_equal(
            rd_grid_get_global_index3(grid, 0, 2, 0),
            rd_grid_get_global_index_from_xyz(grid, x, y, z, 0));

        // Corner 3
        rd_grid_get_cell_corner_xyz3(grid, 2, 2, 0, 3, &x, &y, &z);
        test_assert_int_equal(
            rd_grid_get_global_index3(grid, 2, 2, 0),
            rd_grid_get_global_index_from_xyz(grid, x, y, z, 0));

        // Corner 4
        rd_grid_get_cell_corner_xyz3(grid, 0, 0, 2, 4, &x, &y, &z);
        test_assert_int_equal(
            rd_grid_get_global_index3(grid, 0, 0, 2),
            rd_grid_get_global_index_from_xyz(grid, x, y, z, 0));

        // Corner 5
        rd_grid_get_cell_corner_xyz3(grid, 2, 0, 2, 5, &x, &y, &z);
        test_assert_int_equal(
            rd_grid_get_global_index3(grid, 2, 0, 2),
            rd_grid_get_global_index_from_xyz(grid, x, y, z, 0));

        // Corner 6
        rd_grid_get_cell_corner_xyz3(grid, 0, 2, 2, 6, &x, &y, &z);
        test_assert_int_equal(
            rd_grid_get_global_index3(grid, 0, 2, 2),
            rd_grid_get_global_index_from_xyz(grid, x, y, z, 0));

        // Corner 7
        rd_grid_get_cell_corner_xyz3(grid, 2, 2, 2, 7, &x, &y, &z);
        test_assert_int_equal(
            rd_grid_get_global_index3(grid, 2, 2, 2),
            rd_grid_get_global_index_from_xyz(grid, x, y, z, 0));
    }

    rd_grid_free(grid);
}

int main(int argc, char **argv) {
    rd_grid_type *grid;

    util_install_signals();

    if (argc == 1) {
        grid = rd_grid_alloc_rectangular(6, 6, 6, 1, 2, 3, NULL);
    } else
        grid = rd_grid_alloc(argv[1]);

    test_grid_covering(grid);
    test_contains(grid);

    test_find(grid);
    test_corners();
    rd_grid_free(grid);
    exit(0);
}
