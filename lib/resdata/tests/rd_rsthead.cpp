#include <cstdlib>
#include <ctime>

#include <memory>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_util.hpp>

void test_file(const char *filename, int occurence, bool exists,
               const RSTHead &true_header) {
    int report_step = rd_filename_report_nr(filename);
    rd_file_ptr rst_file = rd::File::open(filename);
    rd_file_enum file_type = rd_get_file_type(filename, NULL, NULL);
    std::shared_ptr<rd::FileView> rst_view;

    if (file_type == RD_RESTART_FILE)
        rst_view = rd_file_get_global_view(rst_file.get());
    else
        rst_view = rd_file_get_global_view(rst_file.get())
                       ->restart_view_from_seqnum_index(occurence);

    if (exists) {
        test_assert_not_NULL(rst_view.get());
        auto rst_head = RSTHead::read(rst_view.get(), report_step);

        if (occurence == 0) {
            auto rst_head0 = RSTHead::read(rst_view.get(), report_step);

            test_assert_true(rst_head == rst_head0);
        }
        test_assert_true(rst_head == true_header);
    } else
        test_assert_NULL(rst_view.get());
}

int main(int argc, char **argv) {
    RSTHead true1{
        /*report_step=*/1,
        /*day=*/1,
        /*year=*/2000,
        /*month=*/1,
        /*sim_time=*/(time_t)946684800,
        /*version=*/100,
        /*phase_sum=*/7,
        /*unit_system=*/RD_METRIC_UNITS,
        /*nx=*/40,
        /*ny=*/64,
        /*nz=*/14,
        /*nactive=*/34770,
        /*nwells=*/3,
        /*niwelz=*/145,
        /*nzwelz=*/3,
        /*nxwelz=*/0,
        /*niconz=*/20,
        /*ncwmax=*/120,
        /*nsconz=*/0,
        /*nxconz=*/0,
        /*nisegz=*/18,
        /*nsegmx=*/1,
        /*nswlmx=*/1,
        /*nlbrmx=*/-1,
        /*nilbrz=*/-1,
        /*nrsegz=*/0,
        /*dualp=*/false,
        /*sim_days=*/0.0,
    };

    RSTHead true2{
        /*report_step=*/5,
        /*day=*/22,
        /*year=*/1990,
        /*month=*/1,
        /*sim_time=*/(time_t)632966400,
        /*version=*/100,
        /*phase_sum=*/7,
        /*unit_system=*/RD_METRIC_UNITS,
        /*nx=*/4,
        /*ny=*/4,
        /*nz=*/4,
        /*nactive=*/64,
        /*nwells=*/3,
        /*niwelz=*/147,
        /*nzwelz=*/3,
        /*nxwelz=*/0,
        /*niconz=*/20,
        /*ncwmax=*/13,
        /*nsconz=*/0,
        /*nxconz=*/0,
        /*nisegz=*/18,
        /*nsegmx=*/1,
        /*nswlmx=*/1,
        /*nlbrmx=*/-1,
        /*nilbrz=*/-1,
        /*nrsegz=*/0,
        /*dualp=*/true,
        /*sim_days=*/21.0,
    };

    const char *unified_file = argv[1];
    const char *Xfile = argv[2];

    test_file(unified_file, 0, true, true1);
    test_file(unified_file, 100, false, true1);
    test_file(Xfile, 0, true, true2);

    exit(0);
}
