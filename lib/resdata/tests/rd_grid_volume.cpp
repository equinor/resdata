#include <cstdlib>
#include <csignal>
#include <cmath>

#include <filesystem>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    fs::path path_case(argv[1]);
    fs::path grid_file = rd::filename(path_case, RD_EGRID_FILE, false, 0);
    std::string init_file =
        rd::filename(path_case, RD_INIT_FILE, false, 0).string();

    std::unique_ptr<rd::File> init = rd::File::open(init_file);
    rd_grid_ptr grid = read_grid(grid_file);
    const rd_kw_type *poro_kw = init->get_kw("PORO", 0);
    const rd_kw_type *porv_kw = init->get_kw("PORV", 0);
    rd_kw_type *multpv = NULL;
    rd_kw_type *NTG = NULL;
    bool error_found = false;

    double total_volume = 0;
    double total_diff = 0;
    int error_count = 0;

    if (init->has_kw("NTG"))
        NTG = init->get_kw("NTG", 0);

    if (init->has_kw("MULTPV"))
        multpv = init->get_kw("MULTPV", 0);

    for (int iactive = 0; iactive < rd_grid_get_nactive(grid.get());
         ++iactive) {
        int iglobal = rd_grid_get_global_index1A(grid.get(), iactive);
        double grid_volume = rd_grid_get_cell_volume1(grid.get(), iglobal);
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

    if (error_found) {
        printf("Error_count: %d / %d \n", error_count,
               rd_grid_get_nactive(grid.get()));
        exit(1);
    } else
        exit(0);
}
