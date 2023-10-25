
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#include <ert/util/hash.hpp>
#include <ert/util/util.h>
#include <ert/util/vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/stringlist.hpp>

#include <resdata/fortio.h>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_rst_file.hpp>
#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_type.hpp>

struct rd_rst_file_struct {
    fortio_type *fortio;
    bool unified;
    bool fmt_file;
};

static rd_rst_file_type *rd_rst_file_alloc(const char *filename) {
    bool unified = rd_unified_file(filename);
    bool fmt_file;
    rd_rst_file_type *rst_file =
        (rd_rst_file_type *)util_malloc(sizeof *rst_file);

    if (rd_fmt_file(filename, &fmt_file)) {
        rst_file->unified = unified;
        rst_file->fmt_file = fmt_file;
        return rst_file;
    } else {
        util_abort("%s: invalid restart filename:%s - could not determine "
                   "formatted/unformatted status\n",
                   __func__, filename);
        return NULL;
    }
}

/**
   Observe that all the open() functions expect that filename conforms
   to the standard conventions, i.e. with extension .FUNRST /
   .UNRST / .Xnnnn / .Fnnnn.
*/

rd_rst_file_type *rd_rst_file_open_read(const char *filename) {
    rd_rst_file_type *rst_file = rd_rst_file_alloc(filename);
    rst_file->fortio =
        fortio_open_reader(filename, rst_file->fmt_file, RD_ENDIAN_FLIP);
    return rst_file;
}

/*
  This function will scan through the file and look for seqnum
  headers, and position the file pointer in the right location to
  start writing data for the report step given by @report_step. The
  file is truncated, so that the filepointer will be at the (new) EOF
  when returning.
*/

rd_rst_file_type *rd_rst_file_open_write_seek(const char *filename,
                                              int report_step) {
    rd_rst_file_type *rst_file = rd_rst_file_alloc(filename);
    offset_type target_pos = 0;
    bool seqnum_found = false;
    rst_file->fortio =
        fortio_open_readwrite(filename, rst_file->fmt_file, RD_ENDIAN_FLIP);
    /*
     If the file does not exist at all the fortio_open_readwrite()
     will fail, we just try again - opening a new file in normal write
     mode, and then immediately returning.
  */
    if (!rst_file->fortio) {
        rst_file->fortio =
            fortio_open_writer(filename, rst_file->fmt_file, RD_ENDIAN_FLIP);
        return rst_file;
    }

    fortio_fseek(rst_file->fortio, 0, SEEK_SET);
    {
        rd_kw_type *work_kw = rd_kw_alloc_new("WORK-KW", 0, RD_INT, NULL);

        while (true) {
            offset_type current_offset = fortio_ftell(rst_file->fortio);

            if (fortio_read_at_eof(rst_file->fortio)) {
                if (seqnum_found)
                    target_pos = current_offset;
                break;
            }

            if (rd_kw_fread_header(work_kw, rst_file->fortio) ==
                RD_KW_READ_FAIL)
                break;

            if (rd_kw_name_equal(work_kw, SEQNUM_KW)) {
                rd_kw_fread_realloc_data(work_kw, rst_file->fortio);
                int file_step = rd_kw_iget_int(work_kw, 0);
                if (file_step >= report_step) {
                    target_pos = current_offset;
                    break;
                }
                seqnum_found = true;
            } else
                rd_kw_fskip_data(work_kw, rst_file->fortio);
        }

        rd_kw_free(work_kw);
    }

    fortio_fseek(rst_file->fortio, target_pos, SEEK_SET);
    fortio_ftruncate_current(rst_file->fortio);
    return rst_file;
}

rd_rst_file_type *rd_rst_file_open_write(const char *filename) {
    rd_rst_file_type *rst_file = rd_rst_file_alloc(filename);
    rst_file->fortio =
        fortio_open_writer(filename, rst_file->fmt_file, RD_ENDIAN_FLIP);
    return rst_file;
}

rd_rst_file_type *rd_rst_file_open_append(const char *filename) {
    rd_rst_file_type *rst_file = rd_rst_file_alloc(filename);
    rst_file->fortio =
        fortio_open_append(filename, rst_file->fmt_file, RD_ENDIAN_FLIP);
    return rst_file;
}

void rd_rst_file_close(rd_rst_file_type *rst_file) {
    fortio_fclose(rst_file->fortio);
    free(rst_file);
}

static void rd_rst_file_fwrite_SEQNUM(rd_rst_file_type *rst_file, int seqnum) {
    rd_kw_type *seqnum_kw = rd_kw_alloc(SEQNUM_KW, 1, RD_INT);
    rd_kw_iset_int(seqnum_kw, 0, seqnum);
    rd_kw_fwrite(seqnum_kw, rst_file->fortio);
    rd_kw_free(seqnum_kw);
}

void rd_rst_file_start_solution(rd_rst_file_type *rst_file) {
    rd_kw_type *startsol_kw = rd_kw_alloc(STARTSOL_KW, 0, RD_MESS);
    rd_kw_fwrite(startsol_kw, rst_file->fortio);
    rd_kw_free(startsol_kw);
}

void rd_rst_file_end_solution(rd_rst_file_type *rst_file) {
    rd_kw_type *endsol_kw = rd_kw_alloc(ENDSOL_KW, 0, RD_MESS);
    rd_kw_fwrite(endsol_kw, rst_file->fortio);
    rd_kw_free(endsol_kw);
}

static rd_kw_type *rd_rst_file_alloc_INTEHEAD(rd_rst_file_type *rst_file,
                                              rd_rsthead_type *rsthead,
                                              int simulator) {
    rd_kw_type *intehead_kw =
        rd_kw_alloc(INTEHEAD_KW, INTEHEAD_RESTART_SIZE, RD_INT);
    rd_kw_scalar_set_int(intehead_kw, 0);

    rd_kw_iset_int(intehead_kw, INTEHEAD_UNIT_INDEX, rsthead->unit_system);
    rd_kw_iset_int(intehead_kw, INTEHEAD_NX_INDEX, rsthead->nx);
    rd_kw_iset_int(intehead_kw, INTEHEAD_NY_INDEX, rsthead->ny);
    rd_kw_iset_int(intehead_kw, INTEHEAD_NZ_INDEX, rsthead->nz);
    rd_kw_iset_int(intehead_kw, INTEHEAD_NACTIVE_INDEX, rsthead->nactive);
    rd_kw_iset_int(intehead_kw, INTEHEAD_PHASE_INDEX, rsthead->phase_sum);

    /* All well properties are hardcoded to zero. */
    {
        int NGMAXZ = 0;
        int NWGMAX = 0;
        int NIGRPZ = 0;
        int NSWLMX = 0;
        int NSEGMX = 0;
        int NISEGZ = 0;

        rd_kw_iset_int(intehead_kw, INTEHEAD_NWELLS_INDEX, rsthead->nwells);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NCWMAX_INDEX, rsthead->ncwmax);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NWGMAX_INDEX, NWGMAX);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NGMAXZ_INDEX, NGMAXZ);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NIWELZ_INDEX, rsthead->niwelz);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NZWELZ_INDEX, rsthead->nzwelz);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NICONZ_INDEX, rsthead->niconz);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NIGRPZ_INDEX, NIGRPZ);

        {
            rd_set_date_values(rsthead->sim_time, &rsthead->day,
                               &rsthead->month, &rsthead->year);
            rd_kw_iset_int(intehead_kw, INTEHEAD_DAY_INDEX, rsthead->day);
            rd_kw_iset_int(intehead_kw, INTEHEAD_MONTH_INDEX, rsthead->month);
            rd_kw_iset_int(intehead_kw, INTEHEAD_YEAR_INDEX, rsthead->year);
        }

        rd_kw_iset_int(intehead_kw, INTEHEAD_IPROG_INDEX, simulator);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NSWLMX_INDEX, NSWLMX);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NSEGMX_INDEX, NSEGMX);
        rd_kw_iset_int(intehead_kw, INTEHEAD_NISEGZ_INDEX, NISEGZ);
    }
    return intehead_kw;
}

static rd_kw_type *rd_rst_file_alloc_LOGIHEAD(int simulator) {
    bool dual_porosity = false;
    bool radial_grid_ECLIPSE100 = false;
    bool radial_grid_ECLIPSE300 = false;

    rd_kw_type *logihead_kw =
        rd_kw_alloc(LOGIHEAD_KW, LOGIHEAD_RESTART_SIZE, RD_BOOL);

    rd_kw_scalar_set_bool(logihead_kw, false);

    if (simulator == INTEHEAD_ECLIPSE100_VALUE)
        rd_kw_iset_bool(logihead_kw, LOGIHEAD_RADIAL100_INDEX,
                        radial_grid_ECLIPSE100);
    else
        rd_kw_iset_bool(logihead_kw, LOGIHEAD_RADIAL300_INDEX,
                        radial_grid_ECLIPSE300);

    rd_kw_iset_bool(logihead_kw, LOGIHEAD_DUALP_INDEX, dual_porosity);
    return logihead_kw;
}

static rd_kw_type *rd_rst_file_alloc_DOUBHEAD(rd_rst_file_type *rst_file,
                                              double days) {
    rd_kw_type *doubhead_kw =
        rd_kw_alloc(DOUBHEAD_KW, DOUBHEAD_RESTART_SIZE, RD_DOUBLE);

    rd_kw_scalar_set_double(doubhead_kw, 0);
    rd_kw_iset_double(doubhead_kw, DOUBHEAD_DAYS_INDEX, days);

    return doubhead_kw;
}

void rd_rst_file_fwrite_header(rd_rst_file_type *rst_file, int seqnum,
                               rd_rsthead_type *rsthead_data) {

    if (rst_file->unified)
        rd_rst_file_fwrite_SEQNUM(rst_file, seqnum);

    {
        rd_kw_type *intehead_kw = rd_rst_file_alloc_INTEHEAD(
            rst_file, rsthead_data, INTEHEAD_ECLIPSE100_VALUE);
        rd_kw_fwrite(intehead_kw, rst_file->fortio);
        rd_kw_free(intehead_kw);
    }

    {
        rd_kw_type *logihead_kw =
            rd_rst_file_alloc_LOGIHEAD(INTEHEAD_ECLIPSE100_VALUE);
        rd_kw_fwrite(logihead_kw, rst_file->fortio);
        rd_kw_free(logihead_kw);
    }

    {
        rd_kw_type *doubhead_kw =
            rd_rst_file_alloc_DOUBHEAD(rst_file, rsthead_data->sim_days);
        rd_kw_fwrite(doubhead_kw, rst_file->fortio);
        rd_kw_free(doubhead_kw);
    }
}

void rd_rst_file_add_kw(rd_rst_file_type *rst_file, const rd_kw_type *rd_kw) {
    rd_kw_fwrite(rd_kw, rst_file->fortio);
}

offset_type rd_rst_file_ftell(const rd_rst_file_type *rst_file) {
    return fortio_ftell(rst_file->fortio);
}
