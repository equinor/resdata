#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include <ecl_fstate.h>
#include <util.h>
#include <ecl_sum.h>



static const char * alloc_wellvar_name(const char *path , const char *well , const char *var) {
  char *out_file = malloc(strlen(path) + 1 + strlen(well) + 1 + strlen(var) + 1);
  sprintf(out_file , "%s/%s.%s" , path , well , var);
  return out_file;
}


static void set_well_var(const char *file , char **_well , char **_var) {
  char *var;
  var = strrchr(file , '.') + 1;
  *_var = malloc(strlen(var) + 1);
  strcpy(*_var , var);


  {
    int n = 0;
    while (file[n] != '.')
      n++;
    
    *_well = malloc(n+1);
    strncpy(*_well , file , n);
    (*_well)[n] = '\0';
  }
}



static void ecl_diag_make_plotfile(int iens1 , int iens2 , const ecl_sum_type **ecl_sum_list , const char *out_path , const char *well , const char *var, bool tecplot) {
  char *hvar     = malloc(strlen(var) + 2);
  char *out_file = alloc_wellvar_name(out_path , well , var);
  FILE *stream;
  int items_written = 0;
  int iens,istep;

  
  {
    bool size_eq = true;
    int size0    = ecl_sum_get_size(ecl_sum_list[0]);
    int iens;
    for (iens = 1; iens < (iens2 - iens1); iens++)
      size_eq = size_eq && (ecl_sum_get_size(ecl_sum_list[iens]) == size0);
    if (! size_eq) {
      for (iens = 0; iens < (iens2 - iens1); iens++)
	printf("member:%3d   %d timestep \n",iens + 1, ecl_sum_get_size(ecl_sum_list[iens]));
      UTIL_ABORT("not all ensemble members equally large");
    }
  }
      
      

  sprintf(hvar     , "%sH" , var);
  stream = fopen(out_file , "w");
  if (tecplot) {
    fprintf(stream , "TITLE=\"%s:%s\"\n",well , var);
    fprintf(stream , "VARIABLES=\"days\" \"history\" ");
    for (iens = iens1; iens <= iens2; iens++) 
      fprintf(stream , "\"mem%02d\"" , iens);
    fprintf(stream , "\n");
    fprintf(stream , "ZONE I=%d DATAPACKING=POINT\n" , ecl_sum_get_size(ecl_sum_list[0]));
  }

  for (istep = 0; istep < ecl_sum_get_size(ecl_sum_list[0]);  istep++) {
    float history_value , time_value, value;
    int index = ecl_sum_iget1(ecl_sum_list[0] , istep , well , var , &value);
    if (index >= 0) {
      ecl_sum_iget1(ecl_sum_list[0] , istep , well , hvar  , &history_value);
      ecl_sum_iget2(ecl_sum_list[0] , istep , 0 , &time_value);
      fprintf(stream , " %9.4e %9.4e " , time_value , history_value);
      for (iens = iens1; iens <= iens2; iens++) {
	ecl_sum_iget2(ecl_sum_list[iens - iens1]     , istep , index , &value);
	fprintf(stream , " %9.4e " , value );
      }
      fprintf(stream , "\n");
      items_written++;
    }
  }
  
  fclose(stream);
  if (items_written == 0) {
    fprintf(stderr,"No mathcing values for well:%s variable:%s  empty file:%s deleted\n",well , var , out_file);
    unlink(out_file);
  }
  free(out_file);
  free(hvar);
}



static ecl_sum_type ** ecl_diag_load_ensemble(int iens1, int iens2 , const char *ens_path , const char *eclbase_dir , const char *eclbase, bool fmt_file, bool unified) {
  ecl_sum_type **ecl_sum_list = calloc(iens2 - iens1 + 1 , sizeof(ecl_sum_type *));
  char spec_file[512];
  int iens;
  int fmt_mode;
  if (fmt_file)
    fmt_mode = ECL_FORMATTED;
  else
    fmt_mode = ECL_BINARY;

  for (iens = iens1; iens <= iens2; iens++) {
    if (fmt_file)
      sprintf(spec_file , "%s/%s%04d/%s-%04d.FSMSPEC" , ens_path , eclbase_dir , iens, eclbase ,iens);
    else
      sprintf(spec_file , "%s/%s%04d/%s-%04d.SMSPEC" , ens_path , eclbase_dir , iens, eclbase ,iens);
    
    if (unified) {
      char data_file[512];
      if (fmt_file)
	sprintf(data_file , "%s/%s%04d/%s-%04d.FUNSMRY" , ens_path , eclbase_dir , iens, eclbase ,iens);
      else
	sprintf(data_file , "%s/%s%04d/%s-%04d.UNSMRY" , ens_path , eclbase_dir , iens, eclbase ,iens);
      printf("Loading file: %s ... ",data_file); fflush(stdout);
      ecl_sum_list[iens - iens1] = ecl_sum_load_unified(spec_file , data_file , fmt_mode , true);
      printf("%d timestep \n",ecl_sum_get_size(ecl_sum_list[iens - iens1]));
    } else {
      int files;
      char _path[512];
      char _base[512];
      char **fileList;
      sprintf(_path , "%s/%s%04d" , ens_path , eclbase_dir , iens);
      sprintf(_base , "%s-%04d"   , eclbase  , iens);
      fileList  = ecl_sum_alloc_filelist(_path , _base , fmt_file , &files);
      printf("Loading from directory: %s ...",_path); fflush(stdout);
      ecl_sum_list[iens - iens1] = ecl_sum_load_multiple(spec_file , files , (const char **) fileList , fmt_mode , true);
      printf("%d timestep \n",ecl_sum_get_size(ecl_sum_list[iens - iens1]));
      util_free_string_list(fileList , files);
    }
  }
  return ecl_sum_list;
}




void ecl_diag_ens(int iens1 , int iens2 , const char *out_path , int nwell , const char **well_list , int nvar , const char **var_list , const char *ens_path , const char *eclbase_dir , const char *eclbase, bool fmt_file, bool unified, bool tecplot) {
  int i,iwell,ivar;
  
  ecl_sum_type **ecl_sum_list = ecl_diag_load_ensemble(iens1 , iens2 , ens_path , eclbase_dir , eclbase , fmt_file , unified);
  util_make_path(out_path);
  for (iwell = 0; iwell < nwell; iwell++) {
    for (ivar = 0; ivar < nvar; ivar++) {
      ecl_diag_make_plotfile(iens1 , iens2 , (const ecl_sum_type **) ecl_sum_list , out_path , well_list[iwell] , var_list[ivar] , tecplot);
    }
  }

  
  for (i=0; i <(iens2 - iens1); i++)
    ecl_sum_free(ecl_sum_list[i]);
  free(ecl_sum_list);
}


static void print_prompt(const char *_prompt , int len) {
  char *prompt;
  if (len < strlen(_prompt))
    prompt = malloc(strlen(_prompt) + 5);
  else
    prompt = malloc(len + 5);
  strcpy(prompt , _prompt);
  {
    int N = len - strlen(prompt);
    int i;
    for (i=0; i < N; i++)
      strcat(prompt , ".");
  }
  strcat(prompt , ": ");
  printf(prompt);
  free(prompt);
}



static void read_bool(const char *prompt , int len , bool *yn) {
  char c;
  do {
    print_prompt(prompt , len);
    fscanf(stdin , "%c" , &c);
  } while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
  if (c == 'n' || c == 'N')
    *yn = false;
  else
    *yn = true;
}


static void read_string(const char *prompt , int len , char *s) {
  print_prompt(prompt , len);
  fscanf(stdin , "%s" , s);
}

static void read_int(const char *prompt , int len , int *i) {
  print_prompt(prompt, len); 
  fscanf(stdin , "%d" , i);
}


char ** fread_alloc_wells(const char *well_file , int *_nwell) {
  FILE *fileH;
  char **well_list = NULL;
  char well[32];
  int iwell;
  int nwell = 0;
  if (util_file_exists(well_file)) {
    int nread;
    fileH = fopen(well_file , "r");
    while ( (nread = fscanf(fileH , "%s" , well)) == 1)
      nwell++;
    well_list = util_alloc_string_list(nwell , 32);
    rewind(fileH);

    iwell = 0;
    while ( (nread = fscanf(fileH , "%s" , well_list[iwell])) == 1)
      iwell++;
    fclose(fileH);
  }
  *_nwell = nwell;
  return well_list;
}


static void ecl_diag_add_subplot(FILE *stream , int prior_size , int posterior_size, double ps , double lw , int history_pt , int prior_lt , int posterior_lt,
				 const char * history_title , const char *prior_title , const char * posterior_title,
				 const char * prior_file , const char *posterior_file , const char *well, const char *var) {
  int iens;

  fprintf(stream,"set pointsize %g\n" , ps);
  fprintf(stream,"set xlabel \"Time (days)\" 0,0.50\n");
  fprintf(stream,"set title \"%s %s\" 0,-0.50\n",well , var);
  fprintf(stream,"plot \"%s\" u 1:2 t \"%s\" w p pt %d ,\\\n" , prior_file , history_title , history_pt);
  for (iens = 0; iens < prior_size; iens++) {
    if (iens == 0)
      fprintf(stream , "\"\" u 1:%d t \"%s\" w l lt %d lw %g,  \\\n", iens+3 , prior_title , prior_lt , lw );
    else
      fprintf(stream , "\"\" u 1:%d t \"\" w l lt %d lw %g,  \\\n", iens+3 , prior_lt , lw );
  }

  for (iens = 0; iens < posterior_size; iens++) {
    if (iens == 0)
      fprintf(stream , "\"%s\" u 1:%d t \"%s\" w l lt %d lw %g,  \\\n", posterior_file , iens+3 , posterior_title , posterior_lt , lw );
    else if (iens == (posterior_size - 1))
      fprintf(stream , "\"\" u 1:%d t \"\" w l lt %d lw %g \\\n", iens+3 , posterior_lt , lw );
    else
      fprintf(stream , "\"\" u 1:%d t \"\" w l lt %d lw %g, \\\n", iens+3 , posterior_lt , lw );
  }

  fprintf(stream , "\n");
}



static void ecl_diag_make_gnuplot(int prior_size , int posterior_size , const char *prior_path , const char *posterior_path , const char *file , const char *plot_path) {
  const char *history_title     = "History";
  const int   history_pt        = 7;
  const char *prior_title       = "Prior";
  const char *posterior_title   = "Posterior";
  const int   prior_lt          = 3;
  const int   posterior_lt      = 1;
  const double lw               = 1.50;
  const double ps               = 0.75;

  char *plot_file      = malloc(strlen(plot_path)  + 1 + strlen(file) + 7);
  char *prior_file     = malloc(strlen(prior_path) + 1 + strlen(file) + 1);
  char *posterior_file = malloc(strlen(prior_path) + 1 + strlen(file) + 1);
  char *ps_file        = malloc(strlen(prior_path) + 1 + strlen(file) + 4);
  char *pdf_file       = malloc(strlen(prior_path) + 1 + strlen(file) + 5);
  char *png_file       = malloc(strlen(prior_path) + 1 + strlen(file) + 5);
  FILE *gplot_stream;

  sprintf(png_file       , "%s/%s.png"   , plot_path      , file);
  sprintf(pdf_file       , "%s/%s.pdf"   , plot_path      , file);
  sprintf(ps_file        , "%s/%s.ps"    , plot_path      , file);
  sprintf(plot_file  	 , "%s/%s.gplot" , plot_path      , file);
  sprintf(prior_file 	 , "%s/%s"       , prior_path     , file);
  sprintf(posterior_file , "%s/%s"       , posterior_path , file);
  
  gplot_stream = fopen(plot_file , "w");
  fprintf(gplot_stream , "set term post enhanced color blacktext solid \"Helvetica\" 14\n");
  fprintf(gplot_stream , "set output \"%s\"\n" , ps_file);


  {
    char *well,*var;
    
    set_well_var(file , &well , &var);
    ecl_diag_add_subplot(gplot_stream , prior_size, posterior_size , ps , lw , history_pt , prior_lt , posterior_lt ,  history_title , prior_title , posterior_title , 
			 prior_file , posterior_file , well , var);
    
    free(well);
    free(var);
  }
  fprintf(gplot_stream, "!convert %s %s \n",ps_file , pdf_file);
  fprintf(gplot_stream, "!convert -rotate 90 %s %s \n",ps_file , png_file);
  fclose(gplot_stream);
  
  
  /*
    free(png_file);
    free(pdf_file);
    free(ps_file);
    free(plot_file);
    free(prior_file);
    free(posterior_file);
  */
}



void ecl_diag_make_gnuplot_interactive() {
  const int prompt_len = 30;
  char prior_path[128];
  char posterior_path[128];
  char plot_path[128];
  char posterior_file[128];
  int  posterior_size , prior_size;
  DIR * priorH;
  struct dirent *dentry;
  
  read_string("Path to prior" ,       	  prompt_len , prior_path);
  read_int("Size of prior ensemble" , 	  prompt_len , &prior_size);
  read_string("Path to posterior",    	  prompt_len , posterior_path);
  read_int("Size of posterior ensemble" , prompt_len , &posterior_size);
  read_string("Path to store plot files", prompt_len , plot_path);

  if ( (priorH = opendir(prior_path)) == NULL) {
    fprintf(stderr,"Opening directory: %s failed - returning" , prior_path);
    return;
  }

  util_make_path(plot_path);
  while ((dentry = readdir (priorH)) != NULL) {
    printf("Ser paa filen: %s \n",dentry->d_name);
    if (!((strcmp(dentry->d_name , ".") == 0) || (strcmp(dentry->d_name , "..") == 0))) {
      sprintf(posterior_file , "%s/%s" , posterior_path , dentry->d_name);
      if (util_file_exists(posterior_file)) 
	ecl_diag_make_gnuplot(prior_size , posterior_size , prior_path , posterior_path , dentry->d_name , plot_path);
      else
	fprintf(stderr,"Warning: file:%s exists only in directory:%s \n",dentry->d_name , prior_path);
    }
  }
  closedir(priorH);

}


void ecl_diag_ens_interactive(const char *eclbase_dir , const char *eclbase_name , bool fmt_file , bool unified) {
#define defvar_N 4
  const int prompt_len = 68;
  char out_path[128];
  char ens_path[128];
  int  i , iens1,iens2,nwell,nvar;
  char prompt[128];
  const char *defvar_list[defvar_N] = {"WOPT" , "WOPR" , "WGOR" , "WWCT"};
  char **well_list;
  char **var_list;
  bool tecplot;
  
  printf("-----------------------------------------------------------------\n");
  read_string("Path to ensemble" , prompt_len , ens_path);
  if (!util_file_exists(ens_path)) {
    fprintf(stderr,"Warning path: %s does not exist - returning. \n",ens_path);
    return;
  }
  
  read_int   ("First ensemble member " , prompt_len , &iens1);
  read_int   ("Last ensemble member " , prompt_len , &iens2);

  read_int   ("Number of wells ...[0 to use all wells in wells.dat]" , prompt_len , &nwell);
  if (nwell == 0) {
    well_list = fread_alloc_wells("wells.dat" , &nwell);
    if (nwell == 0) {
      fprintf(stderr,"Could not find well file: wells.dat - returning \n");
      return;
    }
  } else {
    well_list = util_alloc_string_list(nwell , 64);
    for (i=0; i < nwell; i++) {
      sprintf(prompt , "   Well %2d" , i+1);
      read_string(prompt , prompt_len , well_list[i]);
    }
  }
  
  read_int ("Number of variables  ...[0 to use default set: WOPT WOPR WGOR WWCT]" , prompt_len , &nvar);
  if (nvar == 0) {
    nvar = defvar_N;
    var_list = util_alloc_string_list(nvar , 64);
    for (i=0; i < nvar; i++) 
      strcpy(var_list[i] , defvar_list[i]);
  } else {
    var_list = util_alloc_string_list(nvar , 64);
    for (i=0; i < nvar; i++) {
      sprintf(prompt , "   Var %2d                " , i+1);
      read_string(prompt , prompt_len , var_list[i]);
    }
  }

  read_string("Path to store the results" , prompt_len , out_path);
  read_bool("Add tecplot header" , prompt_len , &tecplot);

  ecl_diag_ens(iens1 , iens2 , out_path , nwell , (const char **) well_list , nvar , (const char **) var_list , ens_path , eclbase_dir , eclbase_name , fmt_file , unified , tecplot);

  util_free_string_list(well_list , nwell);
  util_free_string_list(var_list  , nvar);
}




