#include <util.h>
#include <ecl_sum.h>
#include <stat.h>
#include <double_vector.h>
#include <time_t_vector.h>
#include <config.h>
#include <vector.h>
#include <glob.h>

#define DEFAULT_NUM_INTERP 50



typedef struct {
  ecl_sum_type             * ecl_sum;
  double_vector_type       * interp_data;
  const time_t_vector_type * interp_time;
} sum_case_type;




typedef struct {
  vector_type         * data;
  time_t_vector_type  * interp_time;
  int                   num_interp;
  time_t                start_time;
  time_t                end_time;
} ensemble_type;





sum_case_type * sum_case_fread_alloc( const char * data_file , const time_t_vector_type * interp_time ) {
  sum_case_type * sum_case = util_malloc( sizeof * sum_case , __func__ );

  sum_case->interp_data = double_vector_alloc(0 , 0);
  sum_case->interp_time = interp_time; 
  
  return sum_case;
}



/*****************************************************************/


void ensemble_add_case( ensemble_type * ensemble , const char * data_file ) {
  sum_case_type * sum_case = sum_case_fread_alloc( data_file , ensemble->interp_time );
  vector_append_owned_ref( ensemble->data , sum_case , ecl_sum_free__ );
  if (ensemble->start_time > 0)
    ensemble->start_time = util_time_t_min( ensemble->start_time , ecl_sum_get_start_time( sum_case->ecl_sum ));
  else
    ensemble->start_time = ecl_sum_get_start_time( sum_case->ecl_sum );
  
  ensemble->end_time   = util_time_t_max( ensemble->end_time   , ecl_sum_get_end_time( sum_case->ecl_sum ));
}



void ensemble_init_time_interp( ensemble_type * ensemble ) {
  int i;
  for (i = 0; i < ensemble->num_interp; i++)
    time_t_vector_append( ensemble->interp_time , ensemble->start_time + i * (ensemble->end_time - ensemble->start_time) / (ensemble->num_interp - 1));
}



void ensemble_load_from_glob( enemble_type * ensemble , const char * pattern ) {
  glob_t pglob;
  int    i;
  glob( pattern , GLOB_NOSORT , NULL , &pglob );

  for (i=0; i < pglob.gl_pathc; i++)
    ensemble_add_case( ensemble , pglob.gl_pathv[i]);

  globfree( &glob );
}
  


ensemble_type * ensemble_alloc( config_type * config ) {
  ensemble_type * ensemble = util_malloc( sizeof * ensemble , __func__ );

  ensemble->num_interp  = DEFAULT_NUM_INTERP;
  ensemble->start_time  = -1;
  ensemble->end_time    = -1;
  ensemble->data        = vector_alloc_new();
  ensemble->interp_time = time_t_vector_alloc( 0 , -1 );
  
  return ensemble;
}


void ensemble_init( ensemble_type * ensemble , config_type * config) {

  /*1 : Loading ensembles and settings from the config instance */
  /*1a: Loading the eclipse summary cases. */
  {
    int i,j;
    for (i=0; i < config_get_occurences( config , "CASE_LIST"); i++) {
      const stringlist_type * case_list = config_iget_stringlist_ref( config , "CASE_LIST" , i );
      for (j=0; j < stringlist_get_size( case_list ); j++)
        ensemble_load_from_glob( enemble , stringlist_iget( case_list , j ));
    }
  }

  /*1b: Other config settings */
  if (config_item_set( config , "NUM_INTERP" ))
    ensemble->num_interp  = config_iget_as_int( config , "NUM_INTERP" , 0 , 0 );
  
  
  /*2: Remaining initialization */
  ensemble_init_time_interp( ensemble );
}

/*****************************************************************/

void output_init( hash_type * output , const config_type * config ) {
  
}


/*****************************************************************/

void config_init( config_type * config ) {
  config_item_type * item;

  item = config_add_item( config , "CASE_LIST"      , true , true );
  item = config_add_key_value( config , "NUM_INTERP" , false , CONFIG_INT);
  item = config_add_item( config , "OUTPUT" , true , true );
  config_item_set_argc_minmax( item , 1 , -1 );
}


/*****************************************************************/

int main( int argc , char ** argv ) {
  if (argc != 2)
    usage();
  else {
    hash_type     * output   = hash_alloc();
    ensemble_type * ensemble = ensemble_alloc();
    {
      config_type   * config   = config_alloc( );
      config_init( config );
      config_parse( config , argv[1] );
    
      ensemble_init( ensemble , config );
      output_init( ouptut , config);
      config_free( config );
    }             
  }
}
