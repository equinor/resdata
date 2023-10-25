#include <resdata/rd_sum_index.hpp>

/*
   This file contains all the internalized information from parsing a
   SMSPEC file. In most cases the rd_sum object will contain a
   rd_sum_index_type instance, and the end-user will not have direct
   interaction with the rd_sum_index_type - but for instance when
   working with an ensemble of identical summary results one can use a
   shared rd_sum_index instance.
*/

struct rd_sum_index_type {
    hash_type *well_var_index;            /* Indexes for all well variables. */
    hash_type *well_completion_var_index; /* Indexes for completion indexes .*/
    hash_type *group_var_index;           /* Indexes for group variables.    */
    hash_type *field_var_index;           /* Indexes for field variables.    */
    hash_type *region_var_index;          /* The stored index is an offset.  */
    hash_type *
        misc_var_index; /* Indexes for misceallous variables - typically date. */
    hash_type *block_var_index; /* Indexes for block variables. */

    hash_type *unit_hash; /* Units for the various measurements. */
};
