#include <string.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <math.h>
#include <ecl_fstate.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>
#include <util.h>
#include <time.h>
#include <set.h>
#include <util.h>
#include <vector.h>
#include <int_vector.h>
#include <ecl_smspec.h>
#include <ecl_sum_data.h>

/**
   The ECLIPSE summary data is organised in a header file (.SMSPEC)
   and the actual summary data. The file implements a data structure
   ecl_sum_type which holds ECLIPSE summary data. Most of the actual
   implementation is in separate files ecl_smspec.c for the SMSPEC
   header, and ecl_sum_data for. the actual data.

   Observe that this datastructure is built up around internalizing
   ECLIPSE summary data, the code has NO AMBITION of being able to
   write summary data.
*/






#define ECL_SUM_ID          89067

/*****************************************************************/

struct ecl_sum_struct {
  int                __id;     /* Funny integer used for for "safe" run-time casting. */
  ecl_smspec_type   * smspec;  /* Internalized version of the SMSPEC file. */
  ecl_sum_data_type * data;    /* The data - can be NULL. */
};



/**
   Reads the data from ECLIPSE summary files, can either be a list of
   files BASE.S0000, BASE.S0001, BASE.S0002,.. or one unified
   file. Formatted/unformatted is detected automagically. 
   
   The actual loading is implemented in the ecl_sum_data.c file.
*/


void ecl_sum_fread_realloc_data(ecl_sum_type * ecl_sum , int files , const char ** data_files , bool endian_convert ) {
  if (ecl_sum->data != NULL)
    ecl_sum_free_data( ecl_sum );
  ecl_sum->data   = ecl_sum_data_fread_alloc( ecl_sum->smspec , files , data_files , endian_convert);
}



ecl_sum_type * ecl_sum_fread_alloc(const char *header_file , int files , const char **data_files , bool endian_convert) {
  ecl_sum_type *ecl_sum = util_malloc( sizeof * ecl_sum , __func__);
  ecl_sum->__id   = ECL_SUM_ID;
  ecl_sum->smspec = ecl_smspec_fread_alloc( header_file , endian_convert); 
  ecl_sum->data   = NULL;
  ecl_sum_fread_realloc_data(ecl_sum , files , data_files , endian_convert);
  return ecl_sum;
}


/**
   This function frees the data from the ecl_sum instance and sets the
   data pointer to NULL. The SMSPEC data is still valid, and can be
   reused with calls to ecl_sum_fread_realloc_data().
*/
  
void ecl_sum_free_data( ecl_sum_type * ecl_sum ) {
  ecl_sum_data_free( ecl_sum->data );
  ecl_sum->data = NULL;
}


void ecl_sum_free( ecl_sum_type * ecl_sum ) {
  ecl_sum_free_data( ecl_sum );
  ecl_smspec_free( ecl_sum->smspec );
}


ecl_sum_type * ecl_sum_safe_cast(const void * __ecl_sum) {
  ecl_sum_type * ecl_sum = (ecl_sum_type *) __ecl_sum;
  if (ecl_sum->__id != ECL_SUM_ID)
    util_abort("%s: runtime cast failed - aborting. \n",__func__);
  return ecl_sum;
}


void ecl_sum_free__(void * __ecl_sum) {
  ecl_sum_type * ecl_sum = ecl_sum_safe_cast( __ecl_sum);
  ecl_sum_free( ecl_sum );
}




/**
   This file takes an input file, and loads the corresponding
   summary. The function extracts the path part, and the basename from
   the input file. The extension is not considered (and need to even
   be a valid file). In principle a simulation directory with a given
   basename can contain four different simulation cases:

    * Formatted and unformatted.
    * Unified and not unified.
    
   The program will load the most recent dataset, by looking at the
   modification time stamps of the files.
*/


ecl_sum_type * ecl_sum_fread_alloc_case(const char * input_file , bool endian_convert){
  ecl_sum_type * ecl_sum;
  char * path , * base;
  char * header_file;
  char ** summary_file_list;
  int     files;
  bool    fmt_file , unified;

  util_alloc_file_components( input_file , &path , &base , NULL);
  ecl_util_alloc_summary_files( path , base , &header_file , &summary_file_list , &files , &fmt_file , &unified);
  ecl_sum = ecl_sum_fread_alloc( header_file , files , (const char **) summary_file_list , endian_convert );
  
  free(base);
  util_safe_free(path);
  free(header_file);
  util_free_stringlist( summary_file_list , files );

  return ecl_sum;
}


/*****************************************************************/
/* 
   Here comes lots of access functions - these are mostly thing
   wrapppers around ecl_smspec functions. See more 'extensive'
   documentation in ecl_smspec.c
   
   The functions returning an actual value,
   i.e. ecl_sum_get_well_var() will trustingly call ecl_sum_data_get()
   with whatever indices it gets. If the indices are invalid -
   ecl_sum_data_get() will abort. The abort is the 'correct'
   behaviour, but it is possible to abort in this scope as well, in
   that case more informative error message can be supplied (i.e. the
   well/variable B-33T2/WOPR does not exist, instead of just "invalid
   index" which is the best ecl_sum_data_get() can manage.).
*/

/*****************************************************************/
/* Well variables */

int  	ecl_sum_get_well_var_index(const ecl_sum_type * ecl_sum , const char * well , const char *var) { return ecl_smspec_get_well_var_index(ecl_sum->smspec , well , var); }
bool 	ecl_sum_has_well_var(const ecl_sum_type * ecl_sum , const char * well , const char *var)       { return ecl_smspec_has_well_var(ecl_sum->smspec , well , var); }

double  ecl_sum_get_well_var(const ecl_sum_type * ecl_sum , int ministep , const char * well , const char *var) {
  int index = ecl_sum_get_well_var_index( ecl_sum , well , var );
  return ecl_sum_data_get( ecl_sum->data , ministep , index);
}


/*****************************************************************/
/* Group variables */

int  ecl_sum_get_group_var_index(const ecl_sum_type * ecl_sum , const char * group , const char *var) { return ecl_smspec_get_group_var_index( ecl_sum->smspec , group , var); }
bool ecl_sum_has_group_var(const ecl_sum_type * ecl_sum , const char * group , const char *var)       { return ecl_smspec_has_group_var( ecl_sum->smspec , group , var); }

double  ecl_sum_get_group_var(const ecl_sum_type * ecl_sum , int ministep , const char * group , const char *var) {
  int index = ecl_sum_get_group_var_index( ecl_sum , group , var );
  return ecl_sum_data_get( ecl_sum->data , ministep , index);
}


/*****************************************************************/
/* Field variables */
int  ecl_sum_get_field_var_index(const ecl_sum_type * ecl_sum , const char *var) { return ecl_smspec_get_field_var_index( ecl_sum->smspec , var); }
bool ecl_sum_has_field_var(const ecl_sum_type * ecl_sum , const char *var)       { return ecl_smspec_has_field_var( ecl_sum->smspec , var); }

double ecl_sum_get_field_var(const ecl_sum_type * ecl_sum , int ministep , const char * var) {
  int index = ecl_sum_get_field_var_index( ecl_sum ,  var );
  return ecl_sum_data_get( ecl_sum->data , ministep , index);
}

/*****************************************************************/
/* Block variables */

int  ecl_sum_get_block_var_index(const ecl_sum_type * ecl_sum , const char * block_var , int block_nr) { return ecl_smspec_get_block_var_index( ecl_sum->smspec , block_var , block_nr ); }
bool ecl_sum_has_block_var(const ecl_sum_type * ecl_sum , const char * block_var , int block_nr)       { return ecl_smspec_has_block_var( ecl_sum->smspec , block_var , block_nr ); }
double ecl_sum_get_block_var(const ecl_sum_type * ecl_sum , int ministep , const char * block_var , int block_nr) {
  int index = ecl_sum_get_block_var_index( ecl_sum ,  block_var , block_nr);
  return ecl_sum_data_get( ecl_sum->data , ministep , index);
}


int  ecl_sum_get_block_var_index_ijk(const ecl_sum_type * ecl_sum , const char * block_var , int i, int j , int k ) { 
  return ecl_smspec_get_block_var_index_ijk( ecl_sum->smspec , block_var , i , j , k); 
}

bool ecl_sum_has_block_var_ijk(const ecl_sum_type * ecl_sum , const char * block_var , int i, int j , int k) { 
  return ecl_smspec_has_block_var_ijk( ecl_sum->smspec , block_var , i ,j , k); 
}

double ecl_sum_get_block_var_ijk(const ecl_sum_type * ecl_sum , int ministep , const char * block_var , int i , int j , int k) {
  int index = ecl_sum_get_block_var_index_ijk( ecl_sum ,  block_var , i , j , k);
  return ecl_sum_data_get( ecl_sum->data , ministep , index);
}


/*****************************************************************/
/* Region variables */
/**
   region_nr: [1...num_regions] (NOT C-based indexing)
*/

int  ecl_sum_get_region_var_index(const ecl_sum_type * ecl_sum , int region_nr , const char *var) { return ecl_smspec_get_region_var_index( ecl_sum->smspec , region_nr , var); }
bool ecl_sum_has_region_var(const ecl_sum_type * ecl_sum , int region_nr , const char *var)       { return ecl_smspec_has_region_var( ecl_sum->smspec , region_nr , var); }

double ecl_sum_get_region_var(const ecl_sum_type * ecl_sum , int ministep , int region_nr , const char *var) {
  int index = ecl_sum_get_region_var_index( ecl_sum ,  region_nr , var);
  return ecl_sum_data_get( ecl_sum->data , ministep , index);
}

/*****************************************************************/
/* Misc variables */

int  	ecl_sum_get_misc_var_index(const ecl_sum_type * ecl_sum , const char *var) { return ecl_smspec_get_misc_var_index( ecl_sum->smspec , var ); }
bool 	ecl_sum_has_misc_var(const ecl_sum_type * ecl_sum , const char *var)       { return ecl_smspec_has_misc_var( ecl_sum->smspec , var ); }

double  ecl_sum_get_misc_var(const ecl_sum_type * ecl_sum , int ministep , const char *var) {
  int index = ecl_sum_get_misc_var_index( ecl_sum ,  var);
  return ecl_sum_data_get( ecl_sum->data , ministep , index);
}


/*****************************************************************/
/* Well completion - not fully implemented ?? */

int ecl_sum_get_well_completion_var_index(const ecl_sum_type * ecl_sum , const char * well , const char *var, int cell_nr) { 
  return ecl_smspec_get_well_completion_var_index( ecl_sum->smspec , well , var , cell_nr);
}

bool ecl_sum_has_well_completion_var(const ecl_sum_type * ecl_sum , const char * well , const char *var, int cell_nr)  {
  return ecl_smspec_has_well_completion_var( ecl_sum->smspec , well , var , cell_nr);
}

double ecl_sum_get_well_completion_var(const ecl_sum_type * ecl_sum , int ministep , const char * well , const char *var, int cell_nr)  {
  int index = ecl_sum_get_well_completion_var_index(ecl_sum , well , var , cell_nr);
  return ecl_sum_data_get( ecl_sum->data , ministep , index);
}

/*****************************************************************/
/* General variables ... */

int  ecl_sum_get_general_var_index(const ecl_sum_type * ecl_sum , const char * lookup_kw) { return ecl_smspec_get_general_var_index( ecl_sum->smspec , lookup_kw); }
bool ecl_sum_has_general_var(const ecl_sum_type * ecl_sum , const char * lookup_kw)       { return ecl_smspec_has_general_var( ecl_sum->smspec , lookup_kw); }

double ecl_sum_get_general_var(const ecl_sum_type * ecl_sum , int ministep , const char * lookup_kw) {
  int index = ecl_sum_get_general_var_index(ecl_sum , lookup_kw);
  return ecl_sum_data_get( ecl_sum->data , ministep , index);
}


/*****************************************************************/
/* 
   Here comes a couple of functions relating to the time dimension,
   about reports and ministeps and such things. The functions here in
   this file are just thin wrappers of 'real' functions located in
   ecl_sum_data.c.
*/
   


bool  ecl_sum_has_report_step(const ecl_sum_type * ecl_sum , int report_step ) {
  return ecl_sum_data_has_report_step( ecl_sum->data , report_step );
}


bool  ecl_sum_has_ministep(const ecl_sum_type * ecl_sum , int ministep ) {
  return ecl_sum_data_has_ministep( ecl_sum->data , ministep );
}



void ecl_sum_get_ministep_range(const ecl_sum_type * ecl_sum , int * ministep1, int * ministep2) {
  ecl_sum_data_get_ministep_range(ecl_sum->data , ministep1 , ministep2);
}

   

/*
  Translates a report step to the correspondingly first and last
  ministep. Will set the two ministeps to -1 if the report step is not
  valid.
*/

void ecl_sum_report2ministep_range(const ecl_sum_type * ecl_sum , int report_step , int * ministep1 , int * ministep2 ){
  ecl_sum_data_report2ministep_range( ecl_sum->data , report_step , ministep1 , ministep2);
}


time_t ecl_sum_get_sim_time( const ecl_sum_type * ecl_sum , int ministep ) {
  return ecl_sum_data_get_sim_time( ecl_sum->data , ministep);
}

time_t ecl_sum_get_start_time( const ecl_sum_type * ecl_sum) {
  return ecl_smspec_get_start_time( ecl_sum->smspec );
}


double ecl_sum_get_sim_days( const ecl_sum_type * ecl_sum , int ministep ) {
  return ecl_sum_data_get_sim_days( ecl_sum->data , ministep);
}


/*****************************************************************/
/* This is essentially the summary.x program. */ 

void ecl_sum_fprintf(const ecl_sum_type * ecl_sum , FILE * stream , int nvars , const char ** var_list) {
  int ministep1 , ministep2 , ministep;
  
  ecl_sum_get_ministep_range(ecl_sum , &ministep1 , &ministep2);
  for (ministep = ministep1; ministep <= ministep2; ministep++) {
    if (ecl_sum_has_ministep(ecl_sum , ministep)) {
      int day,month,year,ivar;
      util_set_date_values(ecl_sum_get_sim_time(ecl_sum , ministep) , &day , &month, &year);
      fprintf(stream , "%7.2f   %02d/%02d/%04d   " , ecl_sum_get_sim_days(ecl_sum , ministep) , day , month , year);
      
      for (ivar = 0; ivar < nvars; ivar++)
	fprintf(stream , " %12.3f " , ecl_sum_get_general_var(ecl_sum , ministep , var_list[ivar]));
      
      fprintf(stream , "\n");
    }
  }
}

const char * ecl_sum_get_simulation_case(const ecl_sum_type * ecl_sum) {
  return ecl_smspec_get_simulation_case( ecl_sum->smspec );
}

/*****************************************************************/
/* Legacy shit : */

int ecl_sum_get_num_wells(const ecl_sum_type *ecl_sum) {
  return ecl_smspec_get_num_wells(ecl_sum->smspec);
}

const char ** ecl_sum_get_well_names(const ecl_sum_type * ecl_sum) {
  return ecl_smspec_get_well_names(ecl_sum->smspec);
}


