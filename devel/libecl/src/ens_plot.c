#include <util.h>
#include <vector.h>
#include <ecl_sum.h>
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

  plot_style_type       plot_style;  	    /* LINE | POINT | LINE_POINTS */ 
  plot_color_type       plot_color;  	    /* See available colors in libplot/include/plot_const.h */
  plot_line_style_type  plot_line_style;    /* Line style solid_line | short_dash | long_dash */
  plot_symbol_type      plot_symbol;        /* Integer  - 17 is filled circle ... */
  double                plot_symbol_size;   /* Scale factor for symbol size. */
  double                plot_line_width;    /* Scale factor for line width. */ 
} ens_type;



/** 
    Struct holding basic information used when plotting.
*/

typedef struct plot_info_struct {
  char * plot_path;     /* All the plots will be saved as xxxx files in this directory. */
  char * plot_device;   /* Type of plot file - currently only 'png' is tested. */
  char * viewer;        /* The executable used when displaying the newly created image. */
} plot_info_type;





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


/** 
    Allocating an empty ens_type instance, with all plotting
    attributes initialized to default values.
*/



ens_type * ens_alloc() {
  ens_type * ens  = util_malloc( sizeof * ens, __func__);
  ens->data       = vector_alloc_new();
  
  /* Setting defaults for the plot */
  ens_set_style( ens , LINE );
  ens_set_color( ens , BLUE );
  ens_set_line_style(ens , PLOT_LINESTYLE_SOLID_LINE);
  ens_set_symbol_type(ens , PLOT_SYMBOL_FILLED_CIRCLE); 
  ens_set_symbol_size(ens , 1.0);
  ens_set_line_width(ens , 1.0);
  
  return ens;
}



void ens_free( ens_type * ens) {
  vector_free( ens->data );
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
    
    vector_append_owned_ref( ens->data , ecl_sum_fread_alloc_case( data_file , true) , ecl_sum_free__);
    printf("\n");
    free( base );
    util_safe_free( path );
  } else 
    fprintf(stderr,"Sorry: could not locate case:%s \n",data_file);
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


void ens_set_plot_attributes_batch(void * arg) {
  hash_type * ens_table = (hash_type *) arg;
  //printf("Set plot attributes (color)\n");
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
  
  for (iens = 0; iens < ens_size; iens++) {
    plot_dataset_type * plot_dataset = plot_alloc_new_dataset( plot , label , PLOT_XY );
    const ecl_sum_type * ecl_sum = vector_iget_const( ens->data , iens );
    //int report_step , first_report_step , last_report_step;
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

void plot_finalize(plot_type * plot , plot_info_type * info , const char * plot_file) {
  plot_data(plot);
  plot_free(plot);
  util_vfork_exec(info->viewer , 1 , (const char *[1]) { plot_file } , false , NULL , NULL , NULL , NULL , NULL);
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

void plot_batch(void * arg) {
  // subroutine used in batch mode to plot a vector for a list of ensembles given at stdin
  arg_pack_type * arg_pack   = arg_pack_safe_cast( arg );
  hash_type  * ens_table     = arg_pack_iget_ptr( arg_pack , 0);
  plot_info_type * plot_info = arg_pack_iget_ptr( arg_pack , 1);
  
  printf("Plot summary ensemble(s)\n");
  
  // scan stdin for vector
  
  char * key = util_blocking_alloc_stdin_line(10);
  plot_type     * plot;
  
  char * plot_file;
  
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
  
  const ecl_sum_type * ecl_sum = vector_iget_const( ens->data , 0 );
  
  int first_ministep, last_ministep;
  ecl_sum_get_ministep_range(ecl_sum , &first_ministep , &last_ministep);  
  time_t start_time       = ecl_sum_get_sim_time(ecl_sum , first_ministep ) ; 
  time_t end_time         = ecl_sum_get_sim_time(ecl_sum , last_ministep ) ; 
  //  time_t start_time       = ecl_sum_get_start_time(ecl_sum);

  char ens_name[32];    
  bool complete = false;
  int iens;
  int ens_size;
  bool has_key;

  while (!complete) {
    scanf("%s" , ens_name);
    if(strcmp(ens_name, "_meas_points_") == 0){
      plot_meas_file(plot, start_time);
      continue;
    }  
    if(strcmp(ens_name, "_set_range_") == 0){
      set_range(plot, start_time);
      continue;
    }  
    if(strcmp(ens_name, "_newplotvector_") == 0){
      scanf("%s" , key);
    }  
    else{ 
      if (hash_has_key( ens_table , ens_name)){
	ens = hash_get(ens_table , ens_name);
	
	ens_size = vector_get_size( ens->data );
	has_key = 1;
	// Check if the summary file has the requested key
	for (iens = 0; iens < ens_size; iens++) {
	  ecl_sum = vector_iget_const( ens->data , iens );
	  if(!ecl_sum_has_general_var(ecl_sum , key)){
	    has_key =0;
	    printf("The key %s does not exits\n", key);
	    return;
	  }
	}
	
	if(has_key){
	  plot_ensemble( ens , plot , key);
	}
      }
      else {
	//fprintf(stderr,"Do not have ensemble: \'%s\' \n", ens_name);
	complete = true;
      }
    }
  }
  plot_set_default_timefmt(plot , start_time , end_time);
  plot_finalize(plot , plot_info , plot_file);
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

ens_type * create_named_ensemble(void * arg,   char * ens_name ) {
  hash_type * ens_table = (hash_type *) arg;
  
  if (!hash_has_key( ens_table , ens_name)) {
    ens_type * ens = ens_alloc();
    ens_set_line_width(ens, 1.5);
    printf("Creating named ensemble:%s\n", ens_name);
    hash_insert_hash_owned_ref( ens_table , ens_name , ens , ens_free__);
    return ens;
  }
  return NULL;
}


void create_ensemble_batch(void * arg) {
  hash_type * ens_table = (hash_type *) arg;
  char * line;  
  // scan stdin for ensemble name
  char * ens_name  = util_alloc_stdin_line();
  ens_type  * ens  = create_named_ensemble(ens_table, ens_name);
  
  // scan stdin for valid simulation directory, or a valid eclipse data filename
  
  char * sim_name;
  char * base;
  
  ecl_file_enum file = ECL_DATA_FILE;
  
  while (1){
    line = util_alloc_stdin_line();
    
    if(strcmp(line, "_stop_") == 0){
      return;
    }
    
    // Check if this is a directory
    if(util_is_directory(line)){
      base = ecl_util_alloc_base_guess(line);
      
      sim_name = ecl_util_alloc_filename(line, base, file, 1, 0);
      ens_load_summary(ens , sim_name);
      free(sim_name);
    }
    // Check if this is a file
    else{
      if(util_is_file(line)){
	ens_load_summary(ens , line);
      }
      else{
	printf("Warning: %s is not a valid eclipse summary file or directory\n", line);
      }
    }
    free(line);
  }
  
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
    if(strcmp(argv[1], "-b") == 0){
      char * path = util_blocking_alloc_stdin_line(10);
      
      hash_type * ens_table = hash_alloc();
      plot_info_type * info = plot_info_alloc( path , "png" , VIEWER);
      arg_pack_type * arg_pack = arg_pack_alloc();
      arg_pack_append_ptr( arg_pack , ens_table );
      arg_pack_append_ptr( arg_pack , info );
      char * line;
      free(path);
      
      while (1){
	line = util_blocking_alloc_stdin_line(10);
	util_strupr(line);
	
	if(strcmp(line, "Q") == 0){
	  plot_info_free( info );
	  hash_free( ens_table );
	  exit(1);
	}
	
	if(strcmp(line, "C") == 0){
	  create_ensemble_batch(ens_table);
	}
	
	if(strcmp(line, "P") == 0){
	  plot_batch(arg_pack);
	}
	
	if(strcmp(line, "A") == 0){
	  ens_set_plot_attributes_batch(ens_table);
	}
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
