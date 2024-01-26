/*
  This file contains functionality to write an ECLIPSE INIT file. The
  file does (currently) not contain any datastructures, only
  functions. Essentially this file is only a codifying of the ECLIPSE
  documentation of INIT files.

  The functionality is mainly targeted at saving grid properties like
  PORO, PERMX and FIPNUM. The thermodynamic/relperm properties are
  mainly hardcoded to FALSE (in particular in the
  rd_init_file_alloc_LOGIHEAD() function); this implies that the INIT
  files produced in this way are probably not 100% valid, and can
  certainly not be used to query for e.g. relperm properties.
*/

#include <ert/util/util.hpp>

#include <resdata/fortio.h>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_init_file.hpp>

static rd_kw_type *rd_init_file_alloc_INTEHEAD(const rd_grid_type *rd_grid,
                                               ert_rd_unit_enum unit_system,
                                               int phases, time_t start_date,
                                               int simulator) {
    rd_kw_type *intehead_kw =
        rd_kw_alloc(INTEHEAD_KW, INTEHEAD_INIT_SIZE, RD_INT);
    rd_kw_scalar_set_int(intehead_kw, 0);

    rd_kw_iset_int(intehead_kw, INTEHEAD_UNIT_INDEX, unit_system);
    rd_kw_iset_int(intehead_kw, INTEHEAD_NX_INDEX, rd_grid_get_nx(rd_grid));
    rd_kw_iset_int(intehead_kw, INTEHEAD_NY_INDEX, rd_grid_get_ny(rd_grid));
    rd_kw_iset_int(intehead_kw, INTEHEAD_NZ_INDEX, rd_grid_get_nz(rd_grid));
    rd_kw_iset_int(intehead_kw, INTEHEAD_NACTIVE_INDEX,
                   rd_grid_get_active_size(rd_grid));
    rd_kw_iset_int(intehead_kw, INTEHEAD_PHASE_INDEX, phases);
    {
        int mday, month, year;
        rd_set_date_values(start_date, &mday, &month, &year);
        rd_kw_iset_int(intehead_kw, INTEHEAD_DAY_INDEX, mday);
        rd_kw_iset_int(intehead_kw, INTEHEAD_MONTH_INDEX, month);
        rd_kw_iset_int(intehead_kw, INTEHEAD_YEAR_INDEX, year);
    }
    rd_kw_iset_int(intehead_kw, INTEHEAD_IPROG_INDEX, simulator);

    return intehead_kw;
}

static rd_kw_type *rd_init_file_alloc_LOGIHEAD(int simulator) {
    /*
    This function includes lots of hardcoded options; the main
    intention of writing the INIT file is to store the grid properties
    like e.g. PORO and SATNUM. The relperm/thermodynamics properties
    are just all defaulted to False.

    The documentation
  */
    bool with_RS = false;
    bool with_RV = false;
    bool with_directional_relperm = false;
    bool with_reversible_relperm_ECLIPSE100 = false;
    bool radial_grid_ECLIPSE100 = false;
    bool radial_grid_ECLIPSE300 = false;
    bool with_reversible_relperm_ECLIPSE300 = false;
    bool with_hysterisis = false;
    bool dual_porosity = false;
    bool endpoint_scaling = false;
    bool directional_endpoint_scaling = false;
    bool reversible_endpoint_scaling = false;
    bool alternative_endpoint_scaling = false;
    bool miscible_displacement = false;
    bool scale_water_PC_at_max_sat = false;
    bool scale_gas_PC_at_max_sat = false;

    rd_kw_type *logihead_kw =
        rd_kw_alloc(LOGIHEAD_KW, LOGIHEAD_INIT_SIZE, RD_BOOL);

    rd_kw_scalar_set_bool(logihead_kw, false);

    rd_kw_iset_bool(logihead_kw, LOGIHEAD_RS_INDEX, with_RS);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_RV_INDEX, with_RV);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_DIR_RELPERM_INDEX,
                    with_directional_relperm);

    if (simulator == INTEHEAD_ECLIPSE100_VALUE) {
        rd_kw_iset_bool(logihead_kw, LOGIHEAD_REV_RELPERM100_INDEX,
                        with_reversible_relperm_ECLIPSE100);
        rd_kw_iset_bool(logihead_kw, LOGIHEAD_RADIAL100_INDEX,
                        radial_grid_ECLIPSE100);
    } else {
        rd_kw_iset_bool(logihead_kw, LOGIHEAD_REV_RELPERM300_INDEX,
                        with_reversible_relperm_ECLIPSE300);
        rd_kw_iset_bool(logihead_kw, LOGIHEAD_RADIAL300_INDEX,
                        radial_grid_ECLIPSE300);
    }

    rd_kw_iset_bool(logihead_kw, LOGIHEAD_HYSTERISIS_INDEX, with_hysterisis);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_DUALP_INDEX, dual_porosity);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_ENDPOINT_SCALING_INDEX,
                    endpoint_scaling);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_DIR_ENDPOINT_SCALING_INDEX,
                    directional_endpoint_scaling);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_REV_ENDPOINT_SCALING_INDEX,
                    reversible_endpoint_scaling);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_ALT_ENDPOINT_SCALING_INDEX,
                    alternative_endpoint_scaling);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_MISC_DISPLACEMENT_INDEX,
                    miscible_displacement);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_SCALE_WATER_PC_AT_MAX_SAT_INDEX,
                    scale_water_PC_at_max_sat);
    rd_kw_iset_bool(logihead_kw, LOGIHEAD_SCALE_GAS_PC_AT_MAX_SAT_INDEX,
                    scale_gas_PC_at_max_sat);

    return logihead_kw;
}

static rd_kw_type *rd_init_file_alloc_DOUBHEAD() {
    rd_kw_type *doubhead_kw =
        rd_kw_alloc(DOUBHEAD_KW, DOUBHEAD_INIT_SIZE, RD_DOUBLE);

    rd_kw_scalar_set_double(doubhead_kw, 0);

    return doubhead_kw;
}

/**
   The writing of the PORO field is somewhat special cased; the INIT
   file should contain the PORV keyword with nx*ny*nz elements. The
   cells which are inactive have the PORV volume explicitly set to
   zero; this way the active/inactive status can be inferred from PORV
   field in the INIT file.

   In this code the PORO field is considered to be the fundamental
   quantity, and the PORV field is calculated from PORO and the volume
   of the grid cells. Apart from PORV all the remaining fields in the
   INIT file should have nactive elements.

   If you do not wish this function to be used for the PORV special
   casing you can just pass NULL as the poro_kw in the
   rd_init_file_fwrite_header() function.
 */

static void rd_init_file_fwrite_poro(fortio_type *fortio,
                                     const rd_grid_type *rd_grid,
                                     const rd_kw_type *poro) {
    {
        rd_kw_type *porv =
            rd_kw_alloc(PORV_KW, rd_grid_get_global_size(rd_grid), RD_FLOAT);
        int global_index;
        bool global_poro =
            (rd_kw_get_size(poro) == rd_grid_get_global_size(rd_grid)) ? true
                                                                       : false;
        for (global_index = 0; global_index < rd_grid_get_global_size(rd_grid);
             global_index++) {
            int active_index = rd_grid_get_active_index1(rd_grid, global_index);
            if (active_index >= 0) {
                int poro_index = global_poro ? global_index : active_index;
                rd_kw_iset_float(
                    porv, global_index,
                    rd_kw_iget_float(poro, poro_index) *
                        rd_grid_get_cell_volume1(rd_grid, global_index));
            } else
                rd_kw_iset_float(porv, global_index, 0);
        }
        rd_kw_fwrite(porv, fortio);
        rd_kw_free(porv);
    }

    rd_kw_fwrite(poro, fortio);
}

/*
  If the poro keyword is non NULL this function will write both the
  PORO keyword itself and also calculate the PORV keyword and write
  that.
*/

void rd_init_file_fwrite_header(fortio_type *fortio,
                                const rd_grid_type *rd_grid,
                                const rd_kw_type *poro,
                                ert_rd_unit_enum unit_system, int phases,
                                time_t start_date) {
    int simulator = INTEHEAD_ECLIPSE100_VALUE;
    {
        rd_kw_type *intehead_kw = rd_init_file_alloc_INTEHEAD(
            rd_grid, unit_system, phases, start_date, simulator);
        rd_kw_fwrite(intehead_kw, fortio);
        rd_kw_free(intehead_kw);
    }

    {
        rd_kw_type *logihead_kw = rd_init_file_alloc_LOGIHEAD(simulator);
        rd_kw_fwrite(logihead_kw, fortio);
        rd_kw_free(logihead_kw);
    }

    {
        rd_kw_type *doubhead_kw = rd_init_file_alloc_DOUBHEAD();
        rd_kw_fwrite(doubhead_kw, fortio);
        rd_kw_free(doubhead_kw);
    }

    if (poro) {
        int poro_size = rd_kw_get_size(poro);
        if ((poro_size == rd_grid_get_nactive(rd_grid)) ||
            (poro_size == rd_grid_get_global_size(rd_grid)))
            rd_init_file_fwrite_poro(fortio, rd_grid, poro);
        else
            util_abort("%s: keyword PORO has wrong size:%d  Grid: %d/%d \n",
                       __func__, rd_kw_get_size(poro),
                       rd_grid_get_nactive(rd_grid),
                       rd_grid_get_global_size(rd_grid));
    }
}
