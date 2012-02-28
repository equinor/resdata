/*
   Copyright (C) 2011  Statoil ASA, Norway.

   The file 'ecl_subsidence.c' is part of ERT - Ensemble based
   Reservoir Tool.

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

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <hash.h>
#include <util.h>
#include <vector.h>
#include <ecl_kw.h>
#include <ecl_util.h>
#include <ecl_file.h>
#include <ecl_grid.h>
#include <ecl_region.h>
#include <ecl_subsidence.h>
#include <ecl_grid_cache.h>
#include <ecl_kw_magic.h>
#include <ecl_grav_common.h>


/**
   This file contains datastructures for calculating changes in
   subsidence from compression of reservoirs. The main datastructure is the
   ecl_subsidence_type structure (which is the only structure which is
   exported).
*/


#define POISSON     100


#define SUBSIDENCE_CALC_USE_PRESSURE 128

typedef enum {
  SUBSIDENCE_CALC_PRESSURE = SUBSIDENCE_CALC_USE_PRESSURE
} subsidence_calc_type;


/**
   The ecl_subsidence_struct datastructure is the main structure for
   calculating the subsidence from time lapse ECLIPSE simulations.  
*/

struct ecl_subsidence_struct {
  const ecl_file_type  * init_file;    /* The init file - a shared reference owned by calling scope. */
  ecl_grid_cache_type  * grid_cache;   /* An internal specialized structure to facilitate fast grid lookup. */
  bool                 * aquifer_cell;
  hash_type            * surveys;      /* A hash table containg ecl_subsidence_survey_type instances; one instance
                                          for each interesting time. */
  double               * compressibility; /*total compressibility*/
};




/**
   Data structure representing one subsidence survey.
*/

#define ECL_SUBSIDENCE_SURVEY_ID 88517
struct ecl_subsidence_survey_struct {
  UTIL_TYPE_ID_DECLARATION;
  const ecl_grid_cache_type * grid_cache;
  const bool                * aquifer_cell;   /* Is this cell a numerical aquifer cell - must be disregarded. */
  char                      * name;           /* Name of the survey - arbitrary string. */
  double                    * porv;           /* Reference pore volume */
  double                    * p;              /* Pressure in each grid cell at survey time */
};



/*****************************************************************/


static ecl_subsidence_survey_type * ecl_subsidence_survey_alloc_empty(const ecl_subsidence_type * sub, 
                                                                      const char * name ,
                                                                      subsidence_calc_type calc_type) {
  ecl_subsidence_survey_type * survey = util_malloc( sizeof * survey , __func__ );
  UTIL_TYPE_ID_INIT( survey , ECL_SUBSIDENCE_SURVEY_ID );
  survey->grid_cache   = sub->grid_cache;
  survey->aquifer_cell = sub->aquifer_cell;
  survey->name         = util_alloc_string_copy( name );
  
  if (calc_type & SUBSIDENCE_CALC_USE_PRESSURE) {
    survey->porv = util_malloc( ecl_grid_cache_get_size( sub->grid_cache ) * sizeof * survey->porv , __func__ );
    survey->p    = util_malloc( ecl_grid_cache_get_size( sub->grid_cache ) * sizeof * survey->p , __func__ );
  } else {
    survey->porv = NULL;
    survey->p    = NULL;
  }

  return survey;
}

static UTIL_SAFE_CAST_FUNCTION( ecl_subsidence_survey , ECL_SUBSIDENCE_SURVEY_ID )

static ecl_subsidence_survey_type * ecl_subsidence_survey_alloc_PRESSURE(ecl_subsidence_type * ecl_subsidence ,
                                                                         const ecl_file_type * restart_file ,
                                                                         const char * name ) {
  ecl_subsidence_survey_type * survey = ecl_subsidence_survey_alloc_empty( ecl_subsidence , name , SUBSIDENCE_CALC_PRESSURE);

  ecl_grid_cache_type * grid_cache = ecl_subsidence->grid_cache;
  const int * global_index = ecl_grid_cache_get_global_index( grid_cache );
  const int size = ecl_grid_cache_get_size( grid_cache );
  int active_index;
  ecl_kw_type * init_porv_kw = ecl_file_iget_named_kw( ecl_subsidence->init_file , PORV_KW , 0); /*Global indexing*/
  
  for (active_index = 0; active_index < size; active_index++) 
    survey->porv[ active_index ] = ecl_kw_iget_float( init_porv_kw , global_index[active_index] );

  return survey;
}





static void ecl_subsidence_survey_free( ecl_subsidence_survey_type * subsidence_survey ) {
  free( subsidence_survey->name );
  util_safe_free( subsidence_survey->porv );
  util_safe_free( subsidence_survey->p );
  free( subsidence_survey );
}


static void ecl_subsidence_survey_free__( void * __subsidence_survey ) {
  ecl_subsidence_survey_type * subsidence_survey = ecl_subsidence_survey_safe_cast( __subsidence_survey );
  ecl_subsidence_survey_free( subsidence_survey );
}

/*****************************************************************/

static double ecl_subsidence_survey_eval( const ecl_subsidence_survey_type * base_survey ,
                                          const ecl_subsidence_survey_type * monitor_survey,
                                          ecl_region_type * region ,
                                          double utm_x , double utm_y , double depth, double compressibility) {

  const ecl_grid_cache_type * grid_cache = base_survey->grid_cache;

  const double * xpos   = ecl_grid_cache_get_xpos( grid_cache );
  const double * ypos   = ecl_grid_cache_get_ypos( grid_cache );
  const double * zpos   = ecl_grid_cache_get_zpos( grid_cache );
  int index;

  double deltaz = 0;
  if (region == NULL) {
    const int size = ecl_grid_cache_get_size( grid_cache );
    for (index = 0; index < size; index++) {
      if (!base_survey->aquifer_cell[index]) {

        double base_p = base_survey->p[index];
        double monitor_p = monitor_survey->p[index];
        double base_porv = base_survey->porv[index];

        double delta_porv = compressibility * base_porv * (monitor_p - base_p);

        double dist_x  = (xpos[index] - utm_x );
        double dist_y  = (ypos[index] - utm_y );
        double dist_z  = (zpos[index] - depth );
        double dist    = sqrt( dist_x*dist_x + dist_y*dist_y + dist_z*dist_z );
        /**
           Geertsma formula for subsidence
           1/pi(1-poisson ratio)*dist_z/pow(dist_sq, 1.5)
           We scale to get the result in cm.
        */

        /**
           For numerical precision it might be benficial to use the
           util_kahan_sum() function to do a Kahan summation.
        */
        deltaz += 31.83099*(1-POISSON) * delta_porv * dist_z/( dist * dist * dist );
      }
    }
  } else {
    const int_vector_type * index_vector = ecl_region_get_active_list( region );
    const int size = int_vector_size( index_vector );
    const int * index_list = int_vector_get_const_ptr( index_vector );
    int i;
    for (i = 0; i < size; i++) {
      index = index_list[i];
      if (!base_survey->aquifer_cell[index]) {
        double base_p = base_survey->p[index];
        double monitor_p = monitor_survey->p[index];
        double base_porv = base_survey->porv[index];

        double delta_porv = compressibility * base_porv * (monitor_p - base_p);
        double dist_x  = (xpos[index] - utm_x );
        double dist_y  = (ypos[index] - utm_y );
        double dist_z  = (zpos[index] - depth );
        double dist    = sqrt( dist_x*dist_x + dist_y*dist_y + dist_z*dist_z );

        deltaz += 31.83099*(1-POISSON)*delta_porv* dist_z/(dist * dist * dist );
      }
    }
  }
  return deltaz;
}

/*****************************************************************/
/**
   The grid instance is only used during the construction phase. The
   @init_file object is used by the ecl_subsidence_add_survey_XXX()
   functions; and calling scope must NOT destroy this object before
   all surveys have been added.
*/

ecl_subsidence_type * ecl_subsidence_alloc( const ecl_grid_type * ecl_grid, const ecl_file_type * init_file) {
  ecl_subsidence_type * ecl_subsidence = util_malloc( sizeof * ecl_subsidence , __func__ );
  ecl_subsidence->init_file      = init_file;
  ecl_subsidence->grid_cache     = ecl_grid_cache_alloc( ecl_grid );
  ecl_subsidence->aquifer_cell   = ecl_grav_common_alloc_aquifer_cell( ecl_subsidence->grid_cache , init_file );
  
  ecl_subsidence->surveys        = hash_alloc();
  return ecl_subsidence;
}



static void ecl_subsidence_add_survey__( ecl_subsidence_type * subsidence , const char * name , ecl_subsidence_survey_type * survey) {
  hash_insert_hash_owned_ref( subsidence->surveys , name , survey , ecl_subsidence_survey_free__ );
}

ecl_subsidence_survey_type * ecl_subsidence_add_survey_PRESSURE( ecl_subsidence_type * subsidence , const char * name , const ecl_file_type * restart_file ) {
  ecl_subsidence_survey_type * survey = ecl_subsidence_survey_alloc_PRESSURE( subsidence , restart_file , name );
  ecl_subsidence_add_survey__( subsidence , name , survey );
  return survey;
}

static ecl_subsidence_survey_type * ecl_subsidence_get_survey( const ecl_subsidence_type * subsidence , const char * name) {
  if (name == NULL)
    return NULL;  // Calling scope must determine if this is OK?
  else {
    if (hash_has_key( subsidence->surveys , name))
      return hash_get( subsidence->surveys , name );
    else {
      hash_iter_type * survey_iter = hash_iter_alloc( subsidence->surveys );
      fprintf(stderr,"Survey name:%s not registered. Available surveys are: \n\n     " , name);
      while (!hash_iter_is_complete( survey_iter )) {
        const char * survey = hash_iter_get_next_key( survey_iter );
        fprintf(stderr,"%s ",survey);
      }
      fprintf(stderr,"\n\n");
      hash_iter_free( survey_iter );
      exit(1);
    }
  }
}


double ecl_subsidence_eval( const ecl_subsidence_type * subsidence , const char * base, const char * monitor , ecl_region_type * region , double utm_x, double utm_y , double depth, double compressibility) {
  ecl_subsidence_survey_type * base_survey    = ecl_subsidence_get_survey( subsidence , base );
  ecl_subsidence_survey_type * monitor_survey = ecl_subsidence_get_survey( subsidence , monitor );

  return ecl_subsidence_survey_eval( base_survey , monitor_survey , region , utm_x , utm_y , depth , compressibility);
}

void ecl_subsidence_free( ecl_subsidence_type * ecl_subsidence ) {
  ecl_grid_cache_free( ecl_subsidence->grid_cache );
  free( ecl_subsidence->aquifer_cell );
  hash_free( ecl_subsidence->surveys );
  free( ecl_subsidence );
}

