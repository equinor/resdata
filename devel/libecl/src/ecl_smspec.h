#ifndef __ECL_SMSPEC__
#define __ECL_SMSPEC__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ecl_smspec_struct ecl_smspec_type; 

typedef enum {ecl_smspec_aquifer_var, 
              ecl_smspec_well_var   , 
	      ecl_smspec_region_var , 
	      ecl_smspec_field_var  , 
	      ecl_smspec_group_var  , 
	      ecl_smspec_block_var  , 
	      ecl_smspec_completion_var ,
	      ecl_smspec_local_block_var,
	      ecl_smspec_local_completion_var,
	      ecl_smspec_local_well_var,
	      ecl_smspec_network_var,
	      ecl_smspec_region_2_region_var,
	      ecl_smspec_segment_var,
	      ecl_smspec_misc_var}  ecl_smspec_var_type;




ecl_smspec_type * ecl_smspec_fread_alloc(const char * , bool );
void              ecl_smspec_free( ecl_smspec_type *);
void              ecl_smspec_set_time_info( const ecl_smspec_type *  , const float *  , double *  , time_t * );


#ifdef __cplsplus
}
#endif
#endif
