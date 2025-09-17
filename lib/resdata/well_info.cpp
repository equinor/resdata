#include <time.h>
#include <stdbool.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <ert/util/util.hpp>

#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_util.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_state.hpp>
#include <resdata/well/well_info.hpp>
#include <resdata/well/well_ts.hpp>

/*
  The library libwell contains functionality to read and interpret
  some of the well related keywords from a restart
  file. Roughly speaking the implementation is spread between three
  six datatypes; users of the libwell library are mainly conserned
  with three first.

    well_info_type: This is the container type which holds information
       about all the wells; at all times.

    well_ts_type: The status and properties of a well can typically
       change throughout the simulation; the datatype well_ts_type
       contains a time series for one well.

    well_state_type: The well_state_type datatype contains the
       state/properties of one well at one particular instant of
       time. The well_state.c file contains further documentation of
       the concepts connections, branches and segments.


               WELL1

                | |
                | |
                | |
                | |
           +----| |--------+---------------+---------------+
    LGR1   |   :| |:   :   |               |               |
           +���+| |+���+���+               |    (2,2)      |
           |   :| |:   :   |               |               |
           +���+| |+���+���+               |               |
           |   :| |:   :   |               |               |
           +----| |--------+---------------+---------------+
           |   :| |:   :   |               |   :   :   :   |
           +���+| \__________________________________ -+---+   LGR2
           |   :\______________   ___________________| :   |
           +���+���+���+���+   | |         +---+---+---+---+
           |   :   :   :   |   | |         |   :   :   :   |
           +---------------+---| |---------+---------------+
           |               |   | |         |               |
           |               |   | |         |               |
           |   (0,0)       |   | |         |    (2,0)      |
           |               |   |_|         |               |
           |               |               |               |
           +---------------+---------------+---------------+


     This figure shows the following:

       1. A global grid of 3 x 3 times cell; we assume numbering
          starts with (i,j) = (0,0) in the lower left corner.

       2. The grid contains two LGRs named LGR1 and LGR2
          respectively. The LGR LGR1 has size (4,8) and the LGR2 has
          size (4,4).

       3. The grid contains one well called 'WELL1'; the well has the
          following characteristics[*]:

            a) It is perforated both in LGR1 and LGR2 in addition to
               the global grid.

            b) It has two branches.

          In the well_state instance this will be represented as:

            i) There are three well_path instances corresponding to
               the global grid and the two LGRs respectively.

            ii) The well_path instances corresponding to the two LGRs
                will have one branch only, whereas the well_path
                corrseponding to the global grid will have two branches.

           In pseudo json:

 well_state =  {
                  {well_path : GLOBAL
                            {branch : 0 [ (0,2) , (0,1) , (1,1) , (2,1) ]},
                            {branch : 1 [ (1,0) ] }
                  },
                  {well_path : LGR1
                            {branch : 0 [(1,5),(1,4),(1,3),(1,2),(1,1),(2,1),(3,1)] }
                  }
                  {well_path : LGR2 :
                            {branch : 0 [(0,1),(1,1),(2,1)]}
                  }
               }


 [*] Observe that wells in LGR is quite constrained in ECLIPSE; the
     current libwell implementation handles the illustrated case - but
     it might not be supported by ECLIPSE.


  Limitations
  -----------

     Read-only: The well properties for ECLIPSE is specified through
       the SCHEDULE section of the ECLIPSE datafile. The information
       found in restart files is only for reporting/visaulization+++,
       i.e. the code in libwell can unfortunately not be used for well
       modelling.

     segmented wells: The segment information is used to understand
       the branch structure of the well - but nothing else.

*/

/*
  Usage:
  ------

  1. Create a new well_info_type instance with well_info_alloc().

  2. Add restart data - using one of the three functions:

       - well_info_add_wells()       - rd_file: One report step
       - well_info_add_UNRST_wells() - rd_file: Many report steps
       - well_info_load_rstfile()    - Restart file name; single file or unified

     There are more details about this in a comment section above the
     well_info_add_wells() function.

  3. Query the well_info instance for information about the wells at
     different times; either through the indirect function
     well_info_get_ts() to get the full timeseries for one named well.
     

  4. well_info_free() before you go home.

*/

#define WELL_INFO_TYPE_ID 91777451

struct well_info_struct {
    std::map<std::string, well_ts_type *>
        wells; /* std::map of well_ts_type instances; indexed by well name. */
    std::vector<std::string> well_names; /* A list of all the well names. */
    const rd_grid_type *grid;
};

/**
   The grid pointer is currently not used; but the intention is to use
   it to resolve lgr names.
*/

well_info_type *well_info_alloc(const rd_grid_type *grid) {
    well_info_type *well_info = new well_info_type();
    well_info->grid = grid;
    return well_info;
}

bool well_info_has_well(well_info_type *well_info, const char *well_name) {
    const auto it = well_info->wells.find(well_name);
    if (it == well_info->wells.end())
        return false;
    return true;
}

well_ts_type *well_info_get_ts(const well_info_type *well_info,
                               const char *well_name) {
    return well_info->wells.at(well_name);
}

static void well_info_add_new_ts(well_info_type *well_info,
                                 const char *well_name) {
    well_ts_type *well_ts = well_ts_alloc(well_name);
    well_info->wells[well_name] = well_ts;
    well_info->well_names.push_back(well_name);
}

static void well_info_add_state(well_info_type *well_info,
                                well_state_type *well_state) {
    const char *well_name = well_state_get_name(well_state);
    if (!well_info_has_well(well_info, well_name))
        well_info_add_new_ts(well_info, well_name);

    {
        well_ts_type *well_ts = well_info_get_ts(well_info, well_name);
        well_ts_add_well(well_ts, well_state);
    }
}

/**
   Which function to use for adding wells?

   There are three different functions which can be used to add wells
   to the well_info structure:

     - well_info_add_wells()
     - well_info_add_UNRST_wells()
     - well_info_load_rstfile()

   The two first functions expect an open rd_file instance as input;
   whereas the last funtion expects the name of a restart file as
   input.

   If you need rd_file access to the restart files for another reason
   it might be convenient to use one of the first functions; however
   due to the workings of the rd_file type it might not be entirely
   obvious: The rd_file structure will load the needed keywords on
   demand; the keywords needed to initialize well structures will
   typically not be loaded for other purposes, so the only gain from
   using an existing rd_file instance is that you do not have to
   rebuild the index. The disadvantage of using an existing rd_file
   instance is that after the call to add_wells() the well related
   kewywords will stay in (probaly unused) in memory.

   The three different methods to add restart data can be
   interchganged, and also called repeatedly. All the relevant data is
   internalized in the well_xxx structures; and the restart files can
   be discarded afterwards.
*/

/**
   This function assumes that (sub)select_block() has been used on the
   rd_file instance @rst_file; and the function will load well
   information from the first block available in the file only. To
   load all the well information from a unified restart file it is
   easier to use the well_info_add_UNRST_wells() function; which works
   by calling this function repeatedly.

   This function will go through all the wells by number and call the
   well_state_alloc_from_file() function to create a well state object for each
   well. The well_state_alloc_from_file() function will iterate through all the
   grids and assign well properties corresponding to each of the
   grids, the global grid special-cased to determine is consulted to
   determine the number of wells.
 */

static void well_info_add_wells2(well_info_type *well_info,
                                 rd_file_view_type *rst_view, int report_nr,
                                 bool load_segment_information) {
    bool close_stream = rd_file_view_drop_flag(rst_view, RD_FILE_CLOSE_STREAM);
    rd_rsthead_type *global_header = rd_rsthead_alloc(rst_view, report_nr);
    int well_nr;
    for (well_nr = 0; well_nr < global_header->nwells; well_nr++) {
        well_state_type *well_state =
            well_state_alloc_from_file2(rst_view, well_info->grid, report_nr,
                                        well_nr, load_segment_information);
        if (well_state != NULL)
            well_info_add_state(well_info, well_state);
    }
    rd_rsthead_free(global_header);
    if (close_stream)
        rd_file_view_add_flag(rst_view, RD_FILE_CLOSE_STREAM);
}

static void well_info_add_wells(well_info_type *well_info,
                                rd_file_type *rst_file, int report_nr,
                                bool load_segment_information) {
    well_info_add_wells2(well_info, rd_file_get_active_view(rst_file),
                         report_nr, load_segment_information);
}

/**
   Observe that this function will fail if the rst_file instance
   corresponds to a non-unified restart file, because these files do
   not have the SEQNUM keyword.
*/

static void well_info_add_UNRST_wells2(well_info_type *well_info,
                                       rd_file_view_type *rst_view,
                                       bool load_segment_information) {
    int num_blocks = rd_file_view_get_num_named_kw(rst_view, SEQNUM_KW);
    int block_nr;
    for (block_nr = 0; block_nr < num_blocks; block_nr++) {

        rd_file_view_type *step_view =
            rd_file_view_add_restart_view(rst_view, block_nr, -1, -1, -1);
        const rd_kw_type *seqnum_kw =
            rd_file_view_iget_named_kw(step_view, SEQNUM_KW, 0);
        int report_nr = rd_kw_iget_int(seqnum_kw, 0);

        rd_file_transaction_type *t = rd_file_view_start_transaction(rst_view);
        well_info_add_wells2(well_info, step_view, report_nr,
                             load_segment_information);
        rd_file_view_end_transaction(rst_view, t);
    }
}

static void well_info_add_UNRST_wells(well_info_type *well_info,
                                      rd_file_type *rst_file,
                                      bool load_segment_information) {
    well_info_add_UNRST_wells2(well_info, rd_file_get_global_view(rst_file),
                               load_segment_information);
}

/**
   The @filename argument should be the name of a restart file; in
   unified or not-unified format - if that is not the case we will
   have crash and burn.
*/

void well_info_load_rstfile(well_info_type *well_info, const char *filename,
                            bool load_segment_information) {
    rd_file_type *rd_file = rd_file_open(filename, 0);
    well_info_load_rst_resfile(well_info, rd_file, load_segment_information);
    rd_file_close(rd_file);
}

void well_info_load_rst_resfile(well_info_type *well_info,
                                rd_file_type *rd_file,
                                bool load_segment_information) {
    int report_nr;
    const char *filename = rd_file_get_src_file(rd_file);
    rd_file_enum file_type = rd_get_file_type(filename, NULL, &report_nr);
    if ((file_type == RD_RESTART_FILE) ||
        (file_type == RD_UNIFIED_RESTART_FILE)) {
        if (file_type == RD_RESTART_FILE)
            well_info_add_wells(well_info, rd_file, report_nr,
                                load_segment_information);
        else
            well_info_add_UNRST_wells(well_info, rd_file,
                                      load_segment_information);

    } else
        util_abort("%s: invalid file type: %s - must be a restart file\n",
                   __func__, filename);
}

void well_info_free(well_info_type *well_info) {
    for (const auto &pair : well_info->wells)
        well_ts_free(pair.second);

    delete well_info;
}

int well_info_get_num_wells(const well_info_type *well_info) {
    return well_info->well_names.size();
}

const char *well_info_iget_well_name(const well_info_type *well_info,
                                     int well_index) {
    const std::string &well_name = well_info->well_names[well_index];
    return well_name.c_str();
}
