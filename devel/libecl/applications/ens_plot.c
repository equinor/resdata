#include <util.h>
#include <vector.h>
#include <ecl_sum.h>
#include <ecl_rft_file.h>
#include <hash.h>
#include <menu.h>
#include <plot_const.h>
#include <plot_range.h>
#include <ecl_util.h>
#include <plot.h>
#include <plot_dataset.h>
#include <path_fmt.h>
#include <arg_pack.h>
#include <stdio.h>
#include <string.h>
#include <int_vector.h>
#include <arg_pack.h>
#include <statistics.h>
#include <thread_pool.h>

bool use_viewer = false ; // Global variable to enable backwords compatible behaviour of batch mode
                          // option -b sets use_viewer = true (will start external viewer to show plots)
                          // option -s sets use_viewer = false (slave mode, returns name of plot file on STDOUT)

#define KEY_JOIN_STRING ":"    /* The string used when joining strings to form a gen_key lookup key. */

#define PROMPT_LEN 50

/**
   This is basic datatype to hold the information about one ensemble
   of eclipse simulations. All the simulations ion one ensemble should
   share some characteristica, like all beeing from the prior
   distribution. A plotting session can very well be completed with
   only one ensemble.
*/

typedef struct {
  vector_type         * data;               /* This is a vector ecl_sum instances - actually holding the data. */

  plot_style_type       plot_style;  	    /* LINE | POINTS | LINE_POINTS */ 
  plot_color_type       plot_color;  	    /* See available colors in libplot/include/plot_const.h */
  plot_line_style_type  plot_line_style;    /* Line style: solid_line | short_dash | long_dash */
  plot_symbol_type      plot_symbol;        /* Integer  - 17 is filled circle ... */
  double                plot_symbol_size;   /* Scale factor for symbol size. */
  double                plot_line_width;    /* Scale factor for line width. */ 
  double                sim_length;         /* The length of the _longest_ simulation in the ensemble. */

  /* Everything below line is related to plotting of quantiles. */
  /*-----------------------------------------------------------------*/  
  bool                  use_quantiles;       /* Should this ensemble be plotted as a mean and quantiles - instead of one line pr. member? */ 
  int                   num_sim_days;        /* How many interpolation points to use when resampling the summary data.*/
  double_vector_type  * sim_days;            /* The times where we resample the summary data - given in days since simulation start.*/ 
  double_vector_type  * quantiles;           /* The quantile values we want to plot, i.e [0.10, 0.32, 0.68, 0.90] */
  vector_type         * interp_data;         /* A vector of double_vector instances of the summary data - interpolated to sim_days. */
  vector_type         * quantile_data;       /* The quantiles. */ 
} ens_type;



/** 
    Struct holding basic information used when plotting.
*/

typedef struct plot_info_struct {
  char * plot_path;     /* All the plots will be saved as xxxx files in this directory. */
  char * plot_device;   /* Type of plot file - currently only 'png' is tested. */
  char * viewer;        /* The executable used when displaying the newly created image. */
} plot_info_type;

/*
 * Dialog functions for batch processing (two way communication):
 */

void error_reply(char* message) 
{
  printf("ERROR: %s\n",message) ;
  fflush(stdout) ;
} ;


void warning_reply(char* message) 
{
  printf("WARNING: %s\n",message) ;
  fflush(stdout) ;
} ;


void info_reply(char* message) 
{
  printf("INFO: %s\n",message) ;
  fflush(stdout) ;
} ;



/******************************************************************/
/* Functions manipulating the ensemble type. */

void ens_set_style(ens_type * ens, plot_style_type style) {
  ens->plot_style = style;
}

void ens_set_color(ens_type * ens, plot_color_type color) {
  ens->plot_color = color;
}

void ens_set_line_style(ens_type * ens, plot_line_style_type line_style) {
  ens->plot_line_style = line_style;
}

void ens_set_symbol_type(ens_type * ens, plot_symbol_type symbol_type) {
  ens->plot_symbol = symbol_type;
}

void ens_set_symbol_size(ens_type * ens, double symbol_size) {
  ens->plot_symbol_size = symbol_size;
}

void ens_set_line_width(ens_type * ens, double line_width) {
  ens->plot_line_width = line_width;
}


void ens_use_quantiles( ens_type * ens, bool use_quantiles ) {
  ens->use_quantiles = use_quantiles;
}


void ens_add_quantile( ens_type * ens , double quantile ) {
  double_vector_append( ens->quantiles , quantile );
  vector_append_owned_ref( ens->quantile_data , double_vector_alloc(0,0) , double_vector_free__ );
}

void ens_clear_quantiles( ens_type * ens ) {
  double_vector_reset( ens->quantiles );
  vector_clear( ens->quantile_data );
}

/** 
    Allocating an empty ens_type instance, with all plotting
    attributes initialized to default values.
*/



ens_type * ens_alloc() {
  ens_type * ens   = util_malloc( sizeof * ens, __func__);
  ens->data        = vector_alloc_new();
  /* Quantyile related stuff. */
  ens->quantiles     = double_vector_alloc(0 , 0);
  ens->sim_days      = double_vector_alloc(0 , 0); 
  ens->interp_data   = vector_alloc_new();
  ens->quantile_data = vector_alloc_new();
  ens->sim_length    = 0;
  ens->num_sim_days  = 50;

  /* Setting defaults for the plot */
  ens_set_style( ens , LINE );
  ens_set_color( ens , BLUE );
  ens_set_line_style(ens , PLOT_LINESTYLE_SOLID_LINE);
  ens_set_symbol_type(ens , PLOT_SYMBOL_FILLED_CIRCLE); 
  ens_set_symbol_size(ens , 1.0);
  ens_set_line_width(ens , 1.0);
  ens_use_quantiles( ens , false );
  return ens;
}



void ens_free( ens_type * ens) {
  vector_free( ens->data );
  double_vector_free( ens->quantiles );
  double_vector_free( ens->sim_days  );
  vector_free( ens->interp_data );
  vector_free( ens->quantile_data );
  free(ens);
}


void ens_free__(void * __ens) {
  ens_type * ens = (ens_type *) __ens;
  ens_free (ens );
}


void ens_load_summary(ens_type * ens, const char * data_file) {
  char * base , * path;

  if (util_file_exists(data_file)) {
    util_alloc_file_components( data_file , &path , &base , NULL);
    if (path != NULL)
      printf("Loading case: %s/%s ... ",path , base);
    else
      printf("Loading case: %s .......",base);
    fflush(stdout);
    {
      ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_case( data_file , KEY_JOIN_STRING );
      vector_append_owned_ref( ens->data , ecl_sum , ecl_sum_free__);
      ens->sim_length = util_double_max( ens->sim_length , ecl_sum_get_sim_length( ecl_sum ));
    }
    vector_append_owned_ref( ens->interp_data , double_vector_alloc(0,0) , double_vector_free__ );
    printf("\n");
    free( base );
    util_safe_free( path );
  } else 
    fprintf(stderr,"Sorry: could not locate case:%s \n",data_file);
}


void ens_load_rft(ens_type * ens, const char * data_file) {
  char * base , * path;
  
  
  if (util_file_exists(data_file)) {
    util_alloc_file_components( data_file , &path , &base , NULL);
    char * rft_file = ecl_util_alloc_exfilename( path, base, ECL_RFT_FILE, false, -1 );  
    if(rft_file != NULL){
      printf("Loading case: %s/%s ... ",path , base);
      fflush(stdout);
      vector_append_owned_ref( ens->data , ecl_rft_file_alloc( rft_file) , ecl_rft_file_free__);
      printf("\n");
      free( base );
      util_safe_free( path );
    } else 
      fprintf(stderr,"Sorry: could not locate rft file:%s \n",rft_file);
  } else 
    fprintf(stderr,"Sorry: could not locate case:%s \n",data_file);
}


void ens_load_batch(ens_type* ens, ens_type* ens_rft, const char * data_file) { 
  char* base ;
  char* path ;
  char message[128] ;

  if (util_file_exists(data_file)) {
    util_alloc_file_components( data_file , &path , &base , NULL);
    {
      ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_case( data_file , KEY_JOIN_STRING );
      vector_append_owned_ref( ens->data , ecl_sum , ecl_sum_free__);
      ens->sim_length = util_double_max( ens->sim_length , ecl_sum_get_sim_length( ecl_sum ));
    }
    vector_append_owned_ref( ens->interp_data , double_vector_alloc(0,0) , double_vector_free__ );
    
    char * rft_file = ecl_util_alloc_exfilename( path, base, ECL_RFT_FILE, false, -1 ); 

    if(rft_file != NULL){
      vector_append_owned_ref( ens_rft->data , ecl_rft_file_alloc( rft_file) , ecl_rft_file_free__);
      sprintf(message,"Case %s loaded",base) ;
      info_reply(message) ;
    } else {
      sprintf(message,"No RFT for case %s loaded",base) ;
      warning_reply(message) ;
    } ;

    free( base );
    util_safe_free( path );
  } else {
    sprintf(message,"Case %s not found",base) ;
    error_reply(message) ;
  } 
} 


void ens_load_many(ens_type * ens, path_fmt_type * data_file_fmt , int iens1, int iens2) {
  int iens;
  for (iens = iens1; iens <= iens2; iens++) {
    char * data_file = path_fmt_alloc_path( data_file_fmt , false , iens , iens , iens); /* Substituting up to three %d with the member number. */
    ens_load_summary(ens , data_file);
    free( data_file );
  }
}


void ens_set_plot_attributes(ens_type * ens) {
  int new_color;
  do {
    new_color = util_scanf_int("Color (integer : 0-15 )", PROMPT_LEN);
  } while ((new_color < 0) || (new_color > 15));
  ens_set_color( ens , new_color);
}


void ens_set_plot_attributes_batch(hash_type * ens_table, hash_type * ens_rft_table) {

  char message[128];
  char ens_name[32];
  scanf("%s" , ens_name);
  int new_color;
  char tmp_col[32];
  scanf("%s" , tmp_col);
  util_sscanf_int(tmp_col, &new_color);
  
  if (hash_has_key( ens_table , ens_name)){
    ens_type  * set_ens    = hash_get( ens_table , ens_name);
    if((new_color > -1) && (new_color < 16)){
      ens_set_color( set_ens , new_color);	      
    }	 
  } else {
    sprintf(message,"Unknown ensemble %s",ens_name) ;
    error_reply(message) ;
    return ;
  } 
  
  if (hash_has_key( ens_rft_table , ens_name)){
    ens_type  * set_ens    = hash_get( ens_rft_table , ens_name);
    if((new_color > -1) && (new_color < 16)){
      ens_set_color( set_ens , new_color);	      
    }	 
  } else {
    sprintf(message,"Unknown ensemble %s",ens_name) ;
    error_reply(message) ;
    return ;
  } 

  info_reply("New attributes set") ;
}


/**
   This function will set the quantile properties of an ensemble. The
   main command loop has read a 'Quantiles', and then subsequently gone up
   here. The first argument following the 'Quantiles' should be the name of
   the ensemble, a true or false value (i.e. 1 or 0) as to whether
   quantiles should be used, and if quantiles should be used the
   number of quantiles and their values.
   
   Example1
   --------
   Quantiles <- read in the main loop    
   Prior     <- Name of ensemble this applies to
   1         <- This ensemble should be plotted with quantiles.
   4         <- We want four quantiles
   0.10      <- The four quantile values
   0.32      <- 
   0.68      <-
   0.90      <-

   

   Example2
   --------
   Quantiles <- read in the main loop    
   Prior     <- Name of ensemble this applies to
   0         <- This ensemble should not be plotted with quantiles.

*/

void ens_set_plot_quantile_properties_batch( hash_type * ens_table ) {
  char message[128];
  char ens_name[32];
  scanf("%s" , ens_name);                                             /* Name of ensemble. */
  if (hash_has_key( ens_table , ens_name)){
    ens_type  * ens    = hash_get( ens_table , ens_name);
    int  use_quantiles;
    fscanf(stdin , "%d" , &use_quantiles);                            /* Should quantiles be used? */
    if (use_quantiles == 1) {
      int num_quantiles;
      ens_use_quantiles( ens , true );
      fscanf(stdin , "%d" , &num_quantiles);
      ens_clear_quantiles( ens );
      for (int i = 0; i < num_quantiles; i++) {
        double q;
        fscanf(stdin , "%lg" , &q);
        ens_add_quantile( ens , q );
      }
    } else
      ens_use_quantiles( ens , false);
  } else {
    sprintf(message,"Unknown ensemble %s",ens_name) ;
    error_reply(message) ;
    return;
  } 
  
}


/*****************************************************************/
/** Functions for 'manipulating' the plot_info type. */

void plot_info_set_path(plot_info_type * plot_info , const char * plot_path) {
  plot_info->plot_path = util_realloc_string_copy(plot_info->plot_path , plot_path);
  util_make_path( plot_path );
}


void plot_info_set_device(plot_info_type * plot_info , const char * plot_device) {
  plot_info->plot_device = util_realloc_string_copy(plot_info->plot_device , plot_device);
}

void plot_info_set_viewer(plot_info_type * plot_info , const char * plot_viewer) {
  plot_info->viewer = util_realloc_string_copy(plot_info->viewer , plot_viewer);
}


void plot_info_free( plot_info_type * plot_info) {
  util_safe_free(plot_info->plot_path);
  util_safe_free(plot_info->viewer);
  util_safe_free(plot_info->plot_device);

}


plot_info_type * plot_info_alloc(const char * plot_path , const char * device , const char * viewer) {
  plot_info_type * info = util_malloc( sizeof * info , __func__);
  info->plot_path   = NULL;
  info->plot_device = NULL;
  info->viewer      = NULL;
  
  plot_info_set_path(info , plot_path);
  plot_info_set_device(info , device);
  plot_info_set_viewer(info , viewer);
  
  return info;
}



/*****************************************************************/


void plot_ensemble(const ens_type * ens , plot_type * plot , const char * user_key) {
  const char * label = NULL;
  const int ens_size = vector_get_size( ens->data );
  int iens;


  if (ens->use_quantiles) {
    /* The ensemble is plotted as a mean, and quantiles. */
    
    /* 1: Init simulations days to use for resampling of the summary data. */
    double_vector_reset( ens->sim_days );
    for (int i = 0; i < ens->num_sim_days; i++) {
      double sim_days = i * ens->sim_length / (ens->num_sim_days - 1);
      double_vector_iset( ens->sim_days , i , sim_days);
    }
    
    /* 2: resample all the simulation results to the same times. */
    for (iens = 0; iens < ens_size; iens++) {
      const ecl_sum_type * ecl_sum = vector_iget_const( ens->data , iens );
      ecl_sum_resample_from_sim_days( ecl_sum , ens->sim_days , vector_iget( ens->interp_data , iens ) , user_key );
    }
    
    /* 3: Setting up the plot data for the quantiles. */
    {
      /* 3A: Create the plot_dataset instances and set the properties for the plots. */
      vector_type * quantiles  = vector_alloc_new(); 
      plot_dataset_type * mean = plot_alloc_new_dataset( plot , label , PLOT_XY);

      for (int i=0; i < double_vector_size( ens->quantiles ); i++) {
        plot_dataset_type * quantile_dataset = plot_alloc_new_dataset( plot , label , PLOT_XY);
        vector_append_ref( quantiles , quantile_dataset);
        
        /*
          The plotting style of the quantiles is (currently) quite
          hardcoded:
      
          1. The quantiles are plotted as lines, with linestyle LONG_DASH.
          2. The quantiles are plotted with the same color as the "mother curve" (i.e. the mean).
          3. The quantiles are plotted with a linwidth given by 0.75 times the linewidth of the mean.
        */
        
        plot_dataset_set_style      ( quantile_dataset , LINE );                           
        plot_dataset_set_line_color ( quantile_dataset , ens->plot_color);                 
        plot_dataset_set_line_style ( quantile_dataset , PLOT_LINESTYLE_LONG_DASH);        
        plot_dataset_set_line_width ( quantile_dataset , ens->plot_line_width * 0.75);     
      }

      /* Set the style of the mean. */
      plot_dataset_set_style      ( mean , ens->plot_style);
      plot_dataset_set_line_color ( mean , ens->plot_color);
      plot_dataset_set_point_color( mean , ens->plot_color);
      plot_dataset_set_line_style ( mean , ens->plot_line_style);
      plot_dataset_set_symbol_type( mean , ens->plot_symbol);
      plot_dataset_set_symbol_size( mean , ens->plot_symbol_size);
      plot_dataset_set_line_width ( mean , ens->plot_line_width);
      
      /* 3B: Calculate and add the actual data to plot. */
      {
        double_vector_type * tmp = double_vector_alloc( 0,0);
        for (int i =0; i < double_vector_size( ens->sim_days ); i++) {                    /* looping over the time direction */
          double_vector_reset( tmp );
          for (iens=0; iens < ens_size; iens++) {                                         /* Looping over all the realisations. */
            const double_vector_type * interp_data = vector_iget_const( ens->interp_data , iens );
            double_vector_iset( tmp , iens , double_vector_iget( interp_data , i ));
          }

          /* 
             Now tmp is an ensemble of values resampled to the same
             time; this can be used for quantiles. 
          */
          {
            const ecl_sum_type * ecl_sum = vector_iget_const( ens->data , 0);    

            /* Adding the mean value. */
            plot_dataset_append_point_xy( mean , 
                                          ecl_sum_time_from_days( ecl_sum , double_vector_iget( ens->sim_days , i )) ,    /* Time value */
                                          statistics_mean( tmp ));
            
            /* Adding the quantiles. */
            for (int iq =0; iq < double_vector_size( ens->quantiles ); iq++) {                                                      /* Looping over all the quantiles. */
              double qv;
              plot_dataset_type * data_set = vector_iget( quantiles , iq );
              qv                           = statistics_empirical_quantile( tmp , double_vector_iget( ens->quantiles , iq ));
              plot_dataset_append_point_xy( data_set , 
                                            ecl_sum_time_from_days( ecl_sum , double_vector_iget( ens->sim_days , i )) ,            /* Time value */
                                            qv );                                                                                   /* y-value - the interpolated quantile. */
            }
          }
        }
        double_vector_free( tmp );
      }
      vector_free(quantiles);
    } 
  } else {
    /* The ensemble is plotted as a collection of curves. */
    for (iens = 0; iens < ens_size; iens++) {
      plot_dataset_type * plot_dataset = plot_alloc_new_dataset( plot , label , PLOT_XY );
      const ecl_sum_type * ecl_sum = vector_iget_const( ens->data , iens );
      int ministep, first_ministep, last_ministep;
      ecl_sum_get_ministep_range(ecl_sum , &first_ministep , &last_ministep);  
      
      for (ministep = first_ministep; ministep <= last_ministep; ministep++) { 
        bool ok_mini = ecl_sum_has_ministep( ecl_sum, ministep );
        if (ok_mini ) {
          plot_dataset_append_point_xy( plot_dataset , 
                                        //ecl_sum_get_sim_days( ecl_sum , ministep),
                                        ecl_sum_get_sim_time( ecl_sum , ministep),
                                        ecl_sum_get_general_var( ecl_sum , ministep , user_key ));
        }
      }
      
      plot_dataset_set_style      ( plot_dataset , ens->plot_style);
      plot_dataset_set_line_color ( plot_dataset , ens->plot_color);
      plot_dataset_set_point_color( plot_dataset , ens->plot_color);
      plot_dataset_set_line_style ( plot_dataset , ens->plot_line_style);
      plot_dataset_set_symbol_type( plot_dataset , ens->plot_symbol);
      plot_dataset_set_symbol_size( plot_dataset , ens->plot_symbol_size);
      plot_dataset_set_line_width( plot_dataset , ens->plot_line_width);
    }
  }
}


void plot_rft_ensemble(const ens_type * ens , plot_type * plot , const char * well, time_t survey_time) {
  const char * label = NULL;
  const int ens_size = vector_get_size( ens->data );
  int iens, inode;
  
  const ecl_rft_file_type * ecl_rft = NULL;  
  for (iens = 0; iens < ens_size; iens++) {
    
    plot_dataset_type * plot_dataset = plot_alloc_new_dataset( plot , label , PLOT_XY );
    ecl_rft = vector_iget_const( ens->data , iens );
    const ecl_rft_node_type * ecl_rft_node = ecl_rft_file_get_well_time_rft(ecl_rft, well, survey_time);
    
    
    const int node_size = ecl_rft_node_get_size(ecl_rft_node);
    for (inode =0;inode < node_size;inode++){
      plot_dataset_append_point_xy( plot_dataset , 
				    ecl_rft_node_iget_pressure(ecl_rft_node,inode) ,
				    ecl_rft_node_iget_depth(ecl_rft_node,inode) );
    }
    
    plot_dataset_set_style      ( plot_dataset , ens->plot_style);
    plot_dataset_set_line_color ( plot_dataset , ens->plot_color);
    plot_dataset_set_point_color( plot_dataset , ens->plot_color);
    plot_dataset_set_line_style ( plot_dataset , ens->plot_line_style);
    plot_dataset_set_symbol_type( plot_dataset , ens->plot_symbol);
    plot_dataset_set_symbol_size( plot_dataset , ens->plot_symbol_size);
    plot_dataset_set_line_width( plot_dataset , ens->plot_line_width);
  }
}


void set_range(plot_type * plot, time_t start_time){
  int     num_tokens;
  char ** token_list;
  char  * line;
  
  line = util_blocking_alloc_stdin_line(100);
  util_split_string(line , " " , &num_tokens , &token_list);
  
  int i;
  for (i=0;i<num_tokens-1;i+=2){
    if(strcmp(token_list[i], "XMIN") == 0){
      time_t time = start_time;	 
      util_sscanf_date(token_list[i+1] , &time);
      plot_set_xmin(plot , time);
    }
    else if(strcmp(token_list[i], "XMAX") == 0){
      time_t time = start_time;	 
      util_sscanf_date(token_list[i+1] , &time);
      plot_set_xmax(plot , time);
    }
    else if(strcmp(token_list[i], "YMIN") == 0){
      double  ymin = 0.00;	 
      util_sscanf_double(token_list[i+1] , &ymin);
      plot_set_ymin(plot , ymin);
    }
    else if(strcmp(token_list[i], "YMAX") == 0){
      double  ymax = 0.00;	 
      util_sscanf_double(token_list[i+1] , &ymax);
      plot_set_ymax(plot , ymax);
    }
  }
  
  /** The ymin/ymax values are calculated automatically. */
  
}

void set_range_rft(plot_type * plot){
  int     num_tokens;
  char ** token_list;
  char  * line;
  
  line = util_blocking_alloc_stdin_line(100);
  util_split_string(line , " " , &num_tokens , &token_list);
  
  int i;
  for (i=0;i<num_tokens-1;i+=2){
    if(strcmp(token_list[i], "XMIN") == 0){
      double xmin = 0.00;
      util_sscanf_double(token_list[i+1] , &xmin);
      plot_set_xmin(plot , xmin);
    }
    else if(strcmp(token_list[i], "XMAX") == 0){
      double xmax  = 0.00;
      util_sscanf_double(token_list[i+1] , &xmax);
      plot_set_xmax(plot , xmax);
    }
    else if(strcmp(token_list[i], "YMIN") == 0){
      double  ymin = 0.00;	 
      util_sscanf_double(token_list[i+1] , &ymin);
      plot_set_ymin(plot , ymin);
    }
    else if(strcmp(token_list[i], "YMAX") == 0){
      double  ymax = 0.00;	 
      util_sscanf_double(token_list[i+1] , &ymax);
      plot_set_ymax(plot , ymax);
    }
  }
  
  /** The ymin/ymax values are calculated automatically. */
  
}

double get_rft_depth (hash_type * ens_table, char * well, int i, int j, int k) {
  ens_type * ens;
  
  {
    hash_iter_type * ens_iter = hash_iter_alloc( ens_table );
    ens = hash_iter_get_next_value( ens_iter );
    hash_iter_free( ens_iter );
  }
  const ecl_rft_file_type * ecl_rft = vector_iget_const( ens->data , 0 );
  const ecl_rft_node_type * ecl_rft_node = ecl_rft_file_iget_well_rft(ecl_rft, well, 0);
  const int node_size = ecl_rft_node_get_size(ecl_rft_node);
  int inode;
  int ni,nj,nk;
  for (inode =0;inode < node_size;inode++){
    ecl_rft_node_iget_ijk( ecl_rft_node , inode , &ni , &nj , &nk);
    
    if( i == ni && j==nj && k==nk){
      //double depth = ecl_rft_node_iget_depth(ecl_rft_node,inode);
      return ecl_rft_node_iget_depth(ecl_rft_node,inode);
      
    }
  }
  



  return 0;
  
}

void plot_meas_file(plot_type * plot, time_t start_time){
  bool done = 0;
  double x,x1,x2,y,y1,y2;
  char * error_ptr;
  plot_dataset_type * plot_dataset; 
  time_t time;
  int days;
  while (!done) {
    int     num_tokens;
    char ** token_list;
    char  * line;
    
    line = util_blocking_alloc_stdin_line(10);
    util_split_string(line , " " , &num_tokens , &token_list);
    
    /*
      Tips:

      
      1. Free token_list: util_free_stringlist( token_list , num_tokens);
      2. Parse int/double/...

         if (util_sscanf_double(token_list[??] , &double_value)) 
	    prinftf("Parsed %s -> %g \n",token_list[?+] , double_value);
         else
            printf("Could not interpret %s as double \n",token_list[??]);
	    
    */	    
    
    if (token_list[0] != NULL) {
      if(strcmp(token_list[0], "_stop_") == 0){
	done = 1;
      }
      
      if(strcmp(token_list[0], "xy") == 0){
	util_sscanf_date(token_list[1] , &time);
	util_difftime(start_time, time, &days, NULL, NULL, NULL);
	x = time;
	//x = days;
	
	y = strtod(token_list[2], &error_ptr);
	plot_dataset = plot_alloc_new_dataset( plot , NULL , PLOT_XY );
	plot_dataset_set_style      (plot_dataset , POINTS);
	plot_dataset_append_point_xy(plot_dataset , x , y);
	plot_dataset_set_line_width( plot_dataset , 1.5);
	plot_dataset_set_line_color( plot_dataset , 15);
      }
      
      if(strcmp(token_list[0], "xyy") == 0){
	util_sscanf_date(token_list[1] , &time);
	util_difftime(start_time, time, &days, NULL, NULL, NULL);
	//x = days;
	x = time;
	y1 = strtod(token_list[2], &error_ptr);
	y2 = strtod(token_list[3], &error_ptr);
	
	plot_dataset = plot_alloc_new_dataset( plot , NULL , PLOT_XY1Y2 );
	plot_dataset_append_point_xy1y2(plot_dataset , x , y1, y2);
	plot_dataset_set_line_width( plot_dataset , 1.5);
	plot_dataset_set_line_color( plot_dataset , 15);
      }
      
      if(strcmp(token_list[0], "xxy") == 0){
	x1 = strtod(token_list[1], &error_ptr);
	x2 = strtod(token_list[2], &error_ptr);
	time_t time1 = start_time;	 
	time_t time2 = start_time; 	
	util_inplace_forward_days(&time1 , x1);
	util_inplace_forward_days(&time2 , x2);
	
	y = strtod(token_list[3], &error_ptr);
	plot_dataset = plot_alloc_new_dataset( plot , NULL  , PLOT_X1X2Y );
	plot_dataset_append_point_x1x2y(plot_dataset , time1 , time2, y);
	plot_dataset_set_line_width( plot_dataset , 1.5);
	plot_dataset_set_line_color( plot_dataset , 15);
      }
      
    } else {
      util_forward_line(stdin , &done);
    }
    util_free_stringlist( token_list , num_tokens);
  }
}

void plot_meas_rft_file(plot_type * plot, char * well, hash_type * ens_table){
  bool done = 0;
  int i, j, k;
  double x1, x2;
  plot_dataset_type * plot_dataset; 
  //time_t time;
  //int days;
  while (!done) {
    int     num_tokens;
    char ** token_list;
    char  * line;
    
    line = util_blocking_alloc_stdin_line(10);
    util_split_string(line , " " , &num_tokens , &token_list);
    
    /*
      Tips:

      
      1. Free token_list: util_free_stringlist( token_list , num_tokens);
      2. Parse int/double/...

         if (util_sscanf_double(token_list[??] , &double_value)) 
	    prinftf("Parsed %s -> %g \n",token_list[?+] , double_value);
         else
            printf("Could not interpret %s as double \n",token_list[??]);
	    
    */	    
    
    if (token_list[0] != NULL) {
      if(strcmp(token_list[0], "_stop_") == 0){
	done = 1;
      }
      
      if(strcmp(token_list[0], "rft") == 0){
	util_sscanf_int(token_list[1] , &i);
	util_sscanf_int(token_list[2] , &j);
	util_sscanf_int(token_list[3] , &k);
	util_sscanf_double(token_list[4] , &x1);
	util_sscanf_double(token_list[5] , &x2);
	
	double depth = get_rft_depth(ens_table, well, i, j, k);
	
	if(depth != 0){
	  plot_dataset = plot_alloc_new_dataset( plot , NULL  , PLOT_X1X2Y );
	  plot_dataset_append_point_x1x2y(plot_dataset , x1 , x2, depth);
	  plot_dataset_set_line_width( plot_dataset , 1.5);
	  plot_dataset_set_line_color( plot_dataset , 15);
	}
	else{
	  printf ("The block %i %i %i does not exist in well %s\n", i, j, k, well);
	}
      }
      
    } else {
      util_forward_line(stdin , &done);
    }
    util_free_stringlist( token_list , num_tokens);
  }
}





void plot_finalize(plot_type * plot , plot_info_type * info , const char * plot_file) {
  plot_data(plot);
  plot_free(plot);
  util_fork_exec(info->viewer , 1 , (const char *[1]) { plot_file } , false , NULL , NULL , NULL , NULL , NULL);
}


void plot_all(void * arg) {
  arg_pack_type * arg_pack   = arg_pack_safe_cast( arg );
  hash_type  * ens_table     = arg_pack_iget_ptr( arg_pack , 0);
  plot_info_type * plot_info = arg_pack_iget_ptr( arg_pack , 1);
  {
    plot_type * plot;
    char key[32];
    char * plot_file;
    
    util_printf_prompt("Give key to plot" , PROMPT_LEN , '=' , "=> ");
    scanf("%s" , key);

    plot_file = util_alloc_sprintf("%s/%s.%s" , plot_info->plot_path , key , plot_info->plot_device);
    {
      arg_pack_type * arg_pack = arg_pack_alloc();
      arg_pack_append_ptr( arg_pack , plot_file );
      arg_pack_append_ptr( arg_pack , plot_info->plot_device);

      plot = plot_alloc("PLPLOT" , arg_pack );

      arg_pack_free( arg_pack );
    }
    plot_set_window_size(plot , 640 , 480);
    plot_set_labels(plot , "Time (days)" , key , key);

    {
      bool complete;
      hash_iter_type * ens_iter = hash_iter_alloc( ens_table );
      do {
	complete = hash_iter_is_complete( ens_iter );
	if (!complete) {
	  const ens_type * ens = hash_iter_get_next_value( ens_iter );
	  plot_ensemble( ens , plot , key);
	}
      } while (!complete);
      hash_iter_free( ens_iter );
      plot_finalize(plot , plot_info , plot_file);
    }
    printf("Plot saved in: %s \n",plot_file);
    free( plot_file );
  }
}


void _plot_batch_rft(arg_pack_type* arg_pack, char* inkey){

  // subroutine used in batch mode to plot a summary vector for a list of ensembles given at stdin

  char message[128] ;

  hash_type*      ens_rft_table = arg_pack_iget_ptr( arg_pack , 1);
  plot_info_type* plot_info     = arg_pack_iget_ptr( arg_pack , 2);
  
  plot_type* plot = NULL ;
  
  char * key = inkey;
  
  // split the key in to head, well, date
  int     num_tokens;
  char ** token_list;
  util_split_string(key , ":" , &num_tokens , &token_list);  
  
  if(num_tokens != 3){
    sprintf(message,"The key %s does not exist", key);
    error_reply(message) ;
    return;
  }
 
  char * well = token_list[1];
  char * date = token_list[2];
  time_t survey_time ;
  util_sscanf_date(date , &survey_time) ;  
  
  char * plot_file;
  {
    char * plot_name = util_alloc_sprintf("%s:%s" , token_list[0] , token_list[1]);
    plot_file = util_alloc_filename(plot_info->plot_path , plot_name  , plot_info->plot_device);
    free( plot_name );
  }
  
  {
    arg_pack_type * arg_pack = arg_pack_alloc();
    arg_pack_append_ptr( arg_pack , plot_file );
    arg_pack_append_ptr( arg_pack , plot_info->plot_device);
    plot = plot_alloc("PLPLOT" , arg_pack );
    arg_pack_free( arg_pack );
  }
  plot_set_window_size(plot , 640 , 480);
  plot_set_labels(plot , "Pressure" , "Depth" , key);
  
  
  char ens_name[32];    
  int iens;
  int ens_size;
  
  ens_type * ens;
  {
    hash_iter_type * ens_iter = hash_iter_alloc( ens_rft_table );
    ens = hash_iter_get_next_value( ens_iter );
    hash_iter_free( ens_iter );
  }
  
  
  // Check if there is anything to plot
   if (!ens || !(ens->data) || vector_get_size(ens->data) <= 0) { // Denne satt langt inne !!!!
    error_reply("No ensembles or RFT files to plot\n");
    return;
  }

  const ecl_rft_file_type * ecl_rft = vector_iget_const( ens->data , 0 );
  
  sprintf(message,"Will plot %s",key) ;
  info_reply(message) ;
  
  bool plotempty = true ;
  bool complete = false;
  bool failed = false ;

  while (!complete) {
    scanf("%s" , ens_name);
    
    if(strcmp(ens_name, "_meas_points_") == 0){
      plot_meas_rft_file(plot, well, ens_rft_table);
      plotempty = false ;
      info_reply("Measured values plotted") ;
    } else if(strcmp(ens_name, "_set_range_") == 0){
      set_range_rft(plot);
      info_reply("Range set") ;
    } else if(strcmp(ens_name, "_newplotvector_") == 0){
      char tmpkey[32];      
      scanf("%31s" , tmpkey);
      sprintf(message,"The key %s does not exist\n", tmpkey);
      error_reply(message) ;
      return;
    } else if (strcmp(ens_name, "_stop_") == 0) {
      complete = true ;
    } else if (hash_has_key(ens_rft_table , ens_name)){
      ens = hash_get(ens_rft_table , ens_name);
      
      ens_size = vector_get_size( ens->data );
      // Check if the rft file has the requested well and date
      for (iens = 0; iens < ens_size && !failed; iens++) {
        ecl_rft = vector_iget_const( ens->data , iens );
        if(!ecl_rft_file_has_well(ecl_rft , well)){
          sprintf(message,"The well %s does not exist\n", well);
          error_reply(message) ;
          failed = true ;
        } else {
          // Check if the rft file has the requested survey time
          const ecl_rft_node_type * ecl_rft_node = ecl_rft_file_get_well_time_rft(ecl_rft, well, survey_time);
          if(ecl_rft_node == NULL){
            sprintf(message,"The survey %s in %s does not exist", well, date);
            error_reply(message) ;
            failed = true ;
          }
        }
      }
	
      if (!failed) {
        plot_rft_ensemble( ens , plot , well, survey_time);
        plotempty = false ;
        sprintf(message,"%s plotted",ens_name) ;
        info_reply(message) ;
      } ;
      
    } else {
      sprintf(message,"unknown ensemble %s",ens_name) ;
      error_reply(message) ;
      return ;
    }
  
  } // End while

  if (plot && !plotempty) {
    plot_data(plot);
    plot_free(plot);
    if (use_viewer) {
      util_fork_exec(plot_info->viewer , 1 , (const char *[1]) { plot_file } , false , NULL , NULL , NULL , NULL , NULL);
    } ;
    sprintf(message,"Plot file %s",plot_file) ;
    info_reply(message) ;
  } else {
    error_reply("No data plotted") ;
  } ;

  free( plot_file );
}

 

void _plot_batch_summary(arg_pack_type* arg_pack, char * inkey){

  // subroutine used in batch mode to plot a summary vector for a list of ensembles given at stdin

  char message[128] ;

  hash_type*      ens_table = arg_pack_iget_ptr( arg_pack , 0);
  plot_info_type* plot_info = arg_pack_iget_ptr( arg_pack , 2);
  
  plot_type* plot = NULL ;
  
  char* key = inkey;
  char* plot_file ;
  
  plot_file = util_alloc_sprintf("%s/%s.%s" , plot_info->plot_path , key , plot_info->plot_device);
  
  {
    arg_pack_type * arg_pack = arg_pack_alloc();
    arg_pack_append_ptr( arg_pack , plot_file );
    arg_pack_append_ptr( arg_pack , plot_info->plot_device);
    plot = plot_alloc("PLPLOT" , arg_pack );
    arg_pack_free( arg_pack );
  }
  plot_set_window_size(plot , 640 , 480);
  plot_set_labels(plot , "Date" , key , key);
  
  // get the simulation start time, to be used in plot_meas_file
  ens_type * ens;
  {
    hash_iter_type * ens_iter = hash_iter_alloc( ens_table );
    ens = hash_iter_get_next_value( ens_iter );
    hash_iter_free( ens_iter );
  }
  
  if (!ens || !(ens->data) || vector_get_size(ens->data) <= 0) { // Denne satt langt inne !!!!
    error_reply("No ensembles found") ;
    return ;
  } ;
  const ecl_sum_type * ecl_sum = vector_iget_const( ens->data , 0 );
  
  int first_ministep, last_ministep;
  ecl_sum_get_ministep_range(ecl_sum , &first_ministep , &last_ministep);  
  time_t start_time       = ecl_sum_get_sim_time(ecl_sum , first_ministep ) ; 
  time_t end_time         = ecl_sum_get_sim_time(ecl_sum , last_ministep ) ; 
  //  time_t start_time       = ecl_sum_get_start_time(ecl_sum);

  sprintf(message,"Will plot %s",key) ;
  info_reply(message) ;

  char ens_name[32];    
  bool complete = false;
  bool plotempty = true ;
  bool failed = false ;

  int iens;
  int ens_size;
  
  while (!complete) {
    scanf("%s" , ens_name);

    if(strcmp(ens_name, "_meas_points_") == 0){
      plot_meas_file(plot, start_time);
      plotempty = false ;
      info_reply("Measured values plotted") ;
    } else if(strcmp(ens_name, "_set_range_") == 0){
      set_range(plot, start_time);
      info_reply("Range set") ;
    } else if(strcmp(ens_name, "_newplotvector_") == 0){// ??????????
      scanf("%s" , key);
    } else if (strcmp(ens_name, "_stop_") == 0) {
      complete = true ;
    }  else  if (hash_has_key( ens_table , ens_name)){
      ens = hash_get(ens_table , ens_name);
      
      ens_size = vector_get_size( ens->data );
      // Check if the summary file has the requested key
      for (iens = 0; iens < ens_size && !failed; iens++) {
        ecl_sum = vector_iget_const( ens->data , iens );
        if(!ecl_sum_has_general_var(ecl_sum , key)){
          sprintf(message,"The key %s does not exist in case %i", key,iens); // How to get name
          error_reply(message) ;
          failed = true ;
        }
      } ;

      if (!failed) {
        plotempty = false ;
        plot_ensemble( ens , plot , key);
        sprintf(message,"%s plotted",ens_name) ;
        info_reply(message) ;
      } ;

    } else {
      sprintf(message,"unknown ensemble %s",ens_name) ;
      error_reply(message) ;
      return ;
    }
  
  } // End while
  
  if (plot && !plotempty) {
    plot_set_default_timefmt(plot , start_time , end_time);
    plot_data(plot);
    plot_free(plot);
    if (use_viewer) {
      util_fork_exec(plot_info->viewer , 1 , (const char *[1]) { plot_file } , false , NULL , NULL , NULL , NULL , NULL);
    } ;
    sprintf(message,"Plot file %s",plot_file) ;
    info_reply(message) ;
  } else {
    error_reply("No data plotted") ;
  } ;

  free( plot_file );
}


/******************************************************************/

ens_type * select_ensemble(hash_type * ens_table, const char * prompt) {
  char ens_name[32];
  
  util_printf_prompt(prompt, PROMPT_LEN , '=' , "=> ");
  scanf("%s" , ens_name);
  if (hash_has_key( ens_table , ens_name))
    return hash_get( ens_table , ens_name);
  else {
    fprintf(stderr,"Do not have ensemble: \'%s\' \n", ens_name);
    return NULL;
  }
}

void plot_batch(arg_pack_type* arg) {
  char *  key = util_blocking_alloc_stdin_line(10);
  int     num_tokens;
  char ** token_list;
  
  // scan stdin for vector
  util_split_string(key , ":" , &num_tokens , &token_list);  
  if(strcmp(token_list[0],"RFT") == 0){
    _plot_batch_rft(arg, key);
  } else{
    _plot_batch_summary(arg, key);
  }
}


void add_simulation(void * arg) {
  hash_type * ens_table = (hash_type *) arg;
  ens_type  * ens       = select_ensemble( ens_table , "Which ensemble");
  if (ens != NULL) {
    char data_file[512];
    util_printf_prompt("Give datafile", PROMPT_LEN , '=' , "=> ");
    scanf("%s" , data_file);
    ens_load_summary( ens , data_file );
  } 
}


void scanf_ensemble_numbers(int * iens1 , int * iens2) {
  bool OK = false;

  util_printf_prompt("Ensemble members (xx yy)" , PROMPT_LEN , '=' , "=> ");
  
  while (!OK) {
    char * input = util_alloc_stdin_line();
    const char * current_ptr = input;
    OK = true;

    current_ptr = util_parse_int(current_ptr , iens1 , &OK);
    current_ptr = util_skip_sep(current_ptr , " ,-:" , &OK);
    current_ptr = util_parse_int(current_ptr , iens2 , &OK);
    
    if (!OK) 
      printf("Failed to parse two integers from: \"%s\". Example: \"0 - 19\" to get the 20 first members.\n",input);
    free(input);
  }
}
  


void add_many_simulations(void * arg) {
  hash_type * ens_table = (hash_type *) arg;
  ens_type  * ens       = select_ensemble( ens_table , "Which ensemble");
  if (ens != NULL) {
    path_fmt_type * data_file_fmt;
    int iens1 , iens2;
    char data_file[512];
    util_printf_prompt("Give datafile format (with up to three %d) ", PROMPT_LEN , '=' , "=> ");
    scanf("%s" , data_file);
    data_file_fmt = path_fmt_alloc_path_fmt( data_file );
    
    scanf_ensemble_numbers(&iens1 , &iens2); /* iens1 and iens2 are inclusive */

    ens_load_many( ens , data_file_fmt , iens1 , iens2 );
    path_fmt_free( data_file_fmt );
  } 
}




void create_ensemble(void * arg) {
  hash_type * ens_table = (hash_type *) arg;
  char ens_name[32];
  
  util_printf_prompt("Name of new ensemble", PROMPT_LEN , '=' , "=> ");
  scanf("%s" , ens_name);
  if (!hash_has_key( ens_table , ens_name)) {
    ens_type * ens = ens_alloc();
    hash_insert_hash_owned_ref( ens_table , ens_name , ens , ens_free__);
  }
}


void create_named_ensembles(ens_type** ens, ens_type** ens_rft, hash_type* ens_table, hash_type* ens_rft_table, char* ens_name) {
  char message[128] ;
  if (!hash_has_key( ens_table , ens_name) && !hash_has_key( ens_rft_table , ens_name)) {
    *ens = ens_alloc();
    ens_set_line_width(*ens, 1.5);
    hash_insert_hash_owned_ref( ens_table , ens_name , *ens , ens_free__);

    *ens_rft = ens_alloc();
    ens_set_line_width(*ens_rft, 1.5);    
    ens_set_style(*ens_rft, POINTS);    
    ens_set_symbol_type(*ens_rft , PLOT_SYMBOL_FILLED_CIRCLE); 
    hash_insert_hash_owned_ref( ens_rft_table , ens_name , *ens_rft , ens_free__);

    sprintf(message,"Ensemble %s created", ens_name);
    info_reply(message) ;
  } else {
    sprintf(message,"Ensemble %s already exist",ens_name) ;
    error_reply(message) ;
  } ;
  return ;
} ;


void create_ensemble_batch(hash_type* ens_table, hash_type* ens_rft_table) {

  char message[128] ;
  char * line;  
  // scan stdin for ensemble name
  char * ens_name  = util_alloc_stdin_line();
  
  ens_type* ens     = NULL ;
  ens_type* ens_rft = NULL ;
  create_named_ensembles(&ens, &ens_rft, ens_table, ens_rft_table, ens_name);

  // scan stdin for valid simulation directory, or a valid eclipse data filename
  
  char * sim_name;
  char * base;
  
  ecl_file_enum file = ECL_DATA_FILE;

  printf("Have created ensemble:%s \n",ens_name);

  while (1){
    line = util_alloc_stdin_line();
    
    if(strcmp(line, "_stop_") == 0){
      sprintf(message,"Ensemble %s created",ens_name) ;
      info_reply(message) ;
      return;
    }

    // Check if this is a directory
    if(util_is_directory(line)){
      base = ecl_util_alloc_base_guess(line);
      
      sim_name = ecl_util_alloc_filename(line, base, file, 1, 0);
      ens_load_batch(ens,ens_rft,sim_name) ;
      free(sim_name);
    } else {     // Check if this is a file
      if(util_is_file(line)){
        ens_load_batch(ens,ens_rft,line) ;
      } else {
        sprintf(message,"%s is not a valid eclipse summary file or directory", line);
        error_reply(message) ;
        free(line) ;
        return ;
      }
    } ;
    free(line);
  } ;

  sprintf(message,"Ensemble %s created",ens_name) ;
  info_reply(message) ;
}


void set_plot_attributes(void * arg) {
  hash_type * ens_table = (hash_type *) arg;
  ens_type  * ens       = select_ensemble( ens_table , "Which ensemble");
  if (ens != NULL) 
    ens_set_plot_attributes( ens );
  
}





/*****************************************************************/

void print_usage() {
  printf("***************************************************************************\n");
  printf("  This program is used to plot the summary results from many ECLIPSE\n");
  printf("  simulations in one plot.\n");
  printf("\n");
  printf("  1. When starting the program you can give the path to ECLIPSE data\n");
  printf("     files on the command line - the corresponding summary results will\n");
  printf("     be loaded.\n");
  printf("\n");
  printf("  2. In the menu:\n");
  printf("\n");
  printf("     p: Plot summary ensemble(s): This will ask you for a key to plot,\n");
  printf("  	i.e. RPR:2 for a region, WWCT:OP_3 for the watercut in well OP_3\n");
  printf("  	and FOPR for the field oil production rate - and so on. It will\n");
  printf("  	plot all the ensembles you have loaded.\n");
  printf("\n");
  printf("     c: Create a new ensemble: The simulation results are grouped\n");
  printf("  	together in ensembles. With this command you can make a new\n");
  printf("  	(empty) ensemble.\n");
  printf("\n");
  printf("     n: New simulation case: You can load a new simulation; first you\n");
  printf("  	are prompted for the name of an ensemble, and then afterwards\n");
  printf("  	an ECLIPSE data file which is then loaded.\n");
  printf("\n");
  printf("     m: Add many simulations: This is similar to 'n' - but instead of\n");
  printf("  	giving a data-file you give a (C-based) format string containing\n");
  printf("  	up to three %%d format specifiers - these are replaced with\n");
  printf("  	simulation number, and the corresponding cases are loaded.\n");
  printf("\n");
  printf("     a: Set plot attributes: This gives you the possibility to set plot\n");
  printf("  	attributes for an ensemble. Observe that all the members in the\n");
  printf("  	ensemble are plotted with the same attributes. (Currently only\n");
  printf("  	attribute is color).\n");
  printf("\n");
  printf("***************************************************************************\n");
}


/*****************************************************************/

int main(int argc , char ** argv) {
  
  //setvbuf(stdout, NULL, _IOFBF, 0);
  
  if(argc > 1){
    if(strcmp(argv[1], "-b") == 0 || strcmp(argv[1], "-s") == 0) {
      if (strcmp(argv[1], "-b") == 0) {
        use_viewer = true ;
      } ;
      char * path = util_blocking_alloc_stdin_line(10);
      
      hash_type * ens_table = hash_alloc();
      hash_type * ens_rft_table = hash_alloc();
      plot_info_type * info = plot_info_alloc( path , "png" , VIEWER);
      arg_pack_type * arg_pack = arg_pack_alloc();
      arg_pack_append_ptr( arg_pack , ens_table );
      arg_pack_append_ptr( arg_pack , ens_rft_table );
      arg_pack_append_ptr( arg_pack , info );
      char * line;
      free(path);
      
      while (1){
	line = util_blocking_alloc_stdin_line(10);
	util_strupr(line);
        
        printf("Command:%s \n",line);
	
	if(strcmp(line, "Q") == 0 || strcmp(line, "STOP") == 0 ){

	  plot_info_free( info );
	  hash_free( ens_table );
	  hash_free( ens_rft_table );
	  return 0 ;

	} else if(strcmp(line, "C") == 0){

	  create_ensemble_batch(ens_table, ens_rft_table);

	} else if (strcmp(line, "P") == 0){
          
	  plot_batch(arg_pack);

	} else if (strcmp(line, "A") == 0){
	  ens_set_plot_attributes_batch(ens_table, ens_rft_table);
        } else if (strcmp(line, "QUANTILES") == 0){
	  ens_set_plot_quantile_properties_batch( ens_table );
	} else {

          char message[128] ;
          sprintf(message,"Unknown command %s",line) ;
          error_reply(message) ;
	  plot_info_free( info );
	  hash_free( ens_table );
	  hash_free( ens_rft_table );
	  return 1 ;

        } ;

	free(line);
      }
    }
  }
  
  hash_type * ens_table = hash_alloc();
  plot_info_type * info = plot_info_alloc( "Plot" , "png" , VIEWER);
  
  print_usage();  

  
  {
    int iarg;
    if (argc > 1) {
      printf("Loading realizations in ensemble: \"Ensemble1\"\n");
      ens_type  * ens  = ens_alloc();
      for (iarg = 1; iarg < argc; iarg++)
	ens_load_summary(ens , argv[iarg]);
      hash_insert_hash_owned_ref( ens_table , "Ensemble1" , ens , ens_free__);
    }
  }
  
  {
    menu_type * menu = menu_alloc("E C L I P S E Ensemble plot" , "Quit" , "q");
    arg_pack_type * arg_pack = arg_pack_alloc();
    
    arg_pack_append_ptr( arg_pack , ens_table );
    arg_pack_append_ptr( arg_pack , info );
    menu_add_item(menu , "Plot summary ensemble(s)" , "p" , plot_all , arg_pack , NULL);
    menu_add_separator( menu );
    menu_add_item(menu , "Create new ensemble" , "cC" , create_ensemble , ens_table , NULL);
    menu_add_item(menu , "New simulation case" , "nN" , add_simulation , ens_table , NULL);
    menu_add_item(menu , "Add many simulations" , "mM" , add_many_simulations , ens_table , NULL);
    menu_add_separator( menu );
    menu_add_item(menu , "Set plot attributes (color)" , "aA" , set_plot_attributes , ens_table , NULL);
    menu_run(menu);
    menu_free( menu );
    arg_pack_free( arg_pack );
  }
  
  plot_info_free( info );
  hash_free( ens_table );
  exit(1);
}
