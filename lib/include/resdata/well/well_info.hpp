#pragma once
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/well/well_ts.hpp>

/*
  This file contains functionality to read and interpret
  some of the well related keywords from a restart
  file. Roughly speaking the implementation is spread between three
  six datatypes; users of the interface are mainly concerned
  with three first.

    WellInfo: This is the container type which holds information
       about all the wells; at all times.

    WellTimeLine: The status and properties of a well can typically
       change throughout the simulation; the datatype WellTimeLine
       contains a time series for one well.

    WellState: The WellState datatype contains the
       state/properties of one well at one particular instant of
       time. The well_state.hpp file contains further documentation of
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
     current implementation handles the illustrated case - but
     it might not be supported by ECLIPSE.


  Limitations
  -----------

     Read-only: The well properties for ECLIPSE is specified through
       the SCHEDULE section of the ECLIPSE datafile. The information
       found in restart files is only for reporting/visaulization+++,
       i.e. the code herein can unfortunately not be used for well
       modelling.

     segmented wells: The segment information is used to understand
       the branch structure of the well - but nothing else.

 Usage:
  ------

  1. Create a new WellInfo instance.

  2. Add restart data - using one of the three functions:

       - WellInfo::add_wells()       - rd_file: One report step
       - WellInfo::add_UNRST_wells() - rd_file: Many report steps
       - WellInfo::load_rstfile()    - Restart file name; single file or unified

     There are more details about this in a comment section above the
     well_info::add_wells() function.

  3. Query the well_info instance for information about the wells at
     different times; either through the indirect function
     well_info::get_ts() to get the full timeseries for one named well.

 Which function to use for adding wells?

   There are three different functions which can be used to add wells
   to the WellInfo:

     - WellInfo::add_wells()
     - WellInfo::add_UNRST_wells()
     - WellInfo::load_rstfile()

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

class WellInfo {
    std::map<std::string, std::shared_ptr<WellTimeLine>> wells;
    std::vector<std::string> well_names; /* A list of all the well names. */
    const rd_grid_type *grid;

public:
    /** The grid pointer is currently not used; but the intention is to use
       it to resolve lgr names. */
    explicit WellInfo(const rd_grid_type *grid) : grid(grid) {};
    [[nodiscard]] bool has_well(const std::string &well_name) const {
        return wells.find(well_name) != wells.end();
    }
    [[nodiscard]] std::shared_ptr<WellTimeLine>
    get_ts(const std::string &well_name) const {
        return wells.at(well_name);
    }
    /** The @filename argument should be the name of a restart file; in
       unified or not-unified format - if that is not the case you
       get a invalid_argument exception. */
    void load_rstfile(const std::string &filename,
                      bool load_segment_information);
    void load_rstfile(rd_file_type *rd_file, bool load_segment_information);
    /** Assumes that (sub)select_block() has been used on the
       rd_file instance @rst_file; and the function will load well
       information from the first block available in the file only. To
       load all the well information from a unified restart file it is
       easier to use the well_info_add_UNRST_wells() function; which works
       by calling this function repeatedly.

       This function will go through all the wells by number and create a
       WellState for each well.*/
    void add_wells(rd_file_type *rst_file, int report_nr,
                   bool load_segment_information);
    void add_wells(rd_file_view_type *rst_view, int report_nr,
                   bool load_segment_information);
    /** Will fail if the rst_file instance is a non-unified restart file,
       because these files do not have the SEQNUM keyword. */
    void add_UNRST_wells(rd_file_type *rst_file, bool load_segment_information);
    [[nodiscard]] size_t num_wells() const { return well_names.size(); };
    [[nodiscard]] std::string get_well_name(size_t well_index) const {
        return well_names.at(well_index);
    }
    [[nodiscard]] std::vector<std::string> get_well_names() const {
        return well_names;
    }
    [[nodiscard]] auto begin_ts() const { return wells.cbegin(); }
    [[nodiscard]] auto end_ts() const { return wells.cend(); }
};
