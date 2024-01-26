#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <math.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>

int main(int argc, char **argv) {
    const char *path_case = argv[1];
    char *grid_file =
        rd_alloc_filename(NULL, path_case, RD_EGRID_FILE, false, 0);
    char *init_file =
        rd_alloc_filename(NULL, path_case, RD_INIT_FILE, false, 0);

    rd_file_type *init = rd_file_open(init_file, 0);
    rd_grid_type *grid = rd_grid_alloc(grid_file);
    const rd_kw_type *poro_kw = rd_file_iget_named_kw(init, "PORO", 0);
    const rd_kw_type *porv_kw = rd_file_iget_named_kw(init, "PORV", 0);
    rd_kw_type *multpv = NULL;
    rd_kw_type *NTG = NULL;
    bool error_found = false;

    double total_volume = 0;
    double total_diff = 0;
    int error_count = 0;
    int iactive;

    if (rd_file_has_kw(init, "NTG"))
        NTG = rd_file_iget_named_kw(init, "NTG", 0);

    if (rd_file_has_kw(init, "MULTPV"))
        multpv = rd_file_iget_named_kw(init, "MULTPV", 0);

    for (iactive = 0; iactive < rd_grid_get_nactive(grid); ++iactive) {
        int iglobal = rd_grid_get_global_index1A(grid, iactive);
        double grid_volume = rd_grid_get_cell_volume1(grid, iglobal);
        double eclipse_volume = rd_kw_iget_float(porv_kw, iglobal) /
                                rd_kw_iget_float(poro_kw, iactive);

        if (NTG)
            eclipse_volume /= rd_kw_iget_float(NTG, iactive);

        if (multpv)
            eclipse_volume *= rd_kw_iget_float(multpv, iactive);

        total_volume += grid_volume;
        total_diff += fabs(eclipse_volume - grid_volume);
        if (!util_double_approx_equal__(grid_volume, eclipse_volume, 2.5e-3,
                                        0.00)) {
            double diff = 100 * (grid_volume - eclipse_volume) / eclipse_volume;
            printf("Error in cell: %d V1: %g    V2: %g   diff:%g %% \n",
                   iglobal, grid_volume, eclipse_volume, diff);
            error_count++;
            error_found = true;
        }
    }
    printf("Total volume difference: %g %% \n",
           100 * total_diff / total_volume);

    rd_grid_free(grid);
    rd_file_close(init);
    free(grid_file);
    free(init_file);
    if (error_found) {
        printf("Error_count: %d / %d \n", error_count,
               rd_grid_get_nactive(grid));
        exit(1);
    } else
        exit(0);
}
