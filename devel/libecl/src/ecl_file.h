/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'ecl_file.h' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#ifndef __ECL_FILE_H__
#define __ECL_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <time.h>
#include <ecl_util.h>


typedef struct ecl_file_map_struct ecl_file_map_type;
typedef struct ecl_file_struct ecl_file_type;

ecl_file_type  * ecl_file_open( const char * filename );
void             ecl_file_close( ecl_file_type * ecl_file );
void             ecl_file_free__(void * arg);
ecl_kw_type    * ecl_file_iget_named_kw( const ecl_file_type *  ecl_file , const char * kw , int ith);
ecl_kw_type    * ecl_file_icopy_named_kw( const ecl_file_type * ecl_file , const char * kw, int ith);
ecl_kw_type    * ecl_file_icopy_kw( const ecl_file_type * ecl_file , int index);
bool             ecl_file_has_kw( const ecl_file_type * ecl_file , const char * kw);
int              ecl_file_get_num_named_kw(const ecl_file_type * ecl_file , const char * kw);
int              ecl_file_get_size( const ecl_file_type * ecl_fil );
ecl_kw_type    * ecl_file_iget_kw( const ecl_file_type * ecl_file  , int index);
int              ecl_file_get_num_distinct_kw(const ecl_file_type * ecl_file);
const char     * ecl_file_iget_distinct_kw(const ecl_file_type * ecl_file , int index);
const char     * ecl_file_get_src_file( const ecl_file_type * ecl_file );
int              ecl_file_iget_occurence( const ecl_file_type *  ecl_file , int index);
ecl_version_enum ecl_file_get_ecl_version( const ecl_file_type * file );

ecl_file_type *  ecl_file_fread_alloc_selected_kw(const char * filename ,
                                                 const stringlist_type * kw_list );


ecl_file_type * ecl_file_fread_alloc_selected_kw_fortio(fortio_type * fortio , 
                                                        const stringlist_type * kw_list , 
                                                        const char * stop_kw );

ecl_file_type  * ecl_file_alloc_empty( );
void             ecl_file_fwrite_fortio(const ecl_file_type * ec_file  , fortio_type * fortio , int offset);
void             ecl_file_fwrite(const ecl_file_type * ecl_file , const char * , bool fmt_file );

bool             ecl_file_has_kw_ptr(const ecl_file_type * ecl_file , const ecl_kw_type * ecl_kw);
void             ecl_file_replace_kw( ecl_file_type * ecl_file , ecl_kw_type * old_kw , ecl_kw_type * new_kw , bool insert_copy);
int              ecl_file_get_phases( const ecl_file_type * init_file );
void             ecl_file_fprintf_kw_list( const ecl_file_type * ecl_file , FILE * stream );


/*****************************************************************/
/*               R E S T A R T  F I L E S                        */
ecl_file_type  * ecl_file_fread_alloc_unrst_section(const char * filename , int report_step);
ecl_file_type  * ecl_file_fread_alloc_unrst_section_time( const char * filename , time_t sim_time);
ecl_file_type  * ecl_file_fread_alloc_restart_section(fortio_type * fortio);
ecl_file_type  * ecl_file_copy_restart_section_time_t( const ecl_file_type * src_file , time_t sim_time);
time_t           ecl_file_iget_restart_sim_date( const ecl_file_type * restart_file , int occurence );
int              ecl_file_get_restart_index( const ecl_file_type * restart_file , time_t sim_time);
bool             ecl_file_has_report_step( const ecl_file_type * ecl_file , int report_step);
bool             ecl_file_has_sim_time( const ecl_file_type * ecl_file , time_t sim_time);
ecl_file_map_type * ecl_file_get_unrstmap_report_step( ecl_file_type * ecl_file , int report_step);
ecl_file_map_type * ecl_file_get_unrstmap_time_t( ecl_file_type * ecl_file , time_t sim_time);

/*****************************************************************/
/* Map functions: */
ecl_file_map_type  *  ecl_file_get_blockmap( ecl_file_type * ecl_file , const char * kw , int occurence);
ecl_file_map_type  *  ecl_file_get_global_map(const ecl_file_type * ecl_file);
ecl_file_map_type  *  ecl_file_get_unrstmap( ecl_file_type * ecl_file , int seqnum_nr);
ecl_file_map_type  *  ecl_file_get_unsmrymap( ecl_file_type * ecl_file , int seqhdr_nr);
ecl_kw_type        *  ecl_file_map_iget_named_kw( const ecl_file_map_type *  file_map , const char * kw , int ith);
bool                  ecl_file_map_has_kw( const ecl_file_map_type * file_map, const char * kw);


UTIL_IS_INSTANCE_HEADER( ecl_file )

#ifdef __cplusplus
}
#endif 
#endif
