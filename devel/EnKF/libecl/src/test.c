#include <stdlib.h>
#include <stdio.h>
#include <ecl_sum.h>
#include <ecl_fstate.h>

ecl_sum_type * ecl_load(const char *path, const char *run_path , int i) {
  char spec_file[128];
  char data_file[128];

  sprintf(spec_file , "%s/%s%04d/ECLIPSE.SMSPEC" , path , run_path , i+1);
  sprintf(data_file , "%s/%s%04d/ECLIPSE.UNSMRY" , path , run_path , i+1);
  return ecl_sum_load_unified(spec_file , data_file , ECL_FMT_AUTO , true);
}


void add_subplot(FILE *stream , int ens_size , double ps , double lw , int history_pt , int prior_lt , int posterior_lt,
		 const char * history_title , const char *prior_title , const char * posterior_title,
		 const char *well , const char *var) {
  int iens;

  fprintf(stream,"set pointsize %g\n" , ps);
  fprintf(stream,"set xlabel \"Time (days) 0,0.50\n");
  fprintf(stream,"set title \"%s %s\" 0,-0.50\n",well , var);
  fprintf(stream,"plot \"%s-%s\" u 1:2 t \"%s\" w p pt %d ,\\\n" , well , var , history_title , history_pt);
  for (iens=0; iens < ens_size; iens++) {
    if (iens == 0)
      fprintf(stream , "\"\" u 1:%d t \"%s\" w l lt %d lw %g , \"\" u 1:%d t \"%s\" w l lt %d lw %g , \\\n",
	      iens*2+3 , prior_title , prior_lt , lw , iens*2+4 , posterior_title , posterior_lt , lw);
    else if (iens == (ens_size - 1)) 
      fprintf(stream , "\"\" u 1:%d t \"\" w l lt %d lw %g , \"\" u 1:%d t \"\" w l lt %d lw %g \n",
	      iens*2+3 , prior_lt , lw , iens*2 + 4 , posterior_lt , lw);
    else
      fprintf(stream , "\"\" u 1:%d t \"\" w l lt %d lw %g , \"\" u 1:%d t \"\" w l lt %d lw %g , \\\n",
	      iens*2+3 , prior_lt , lw , iens*2 + 4 , posterior_lt , lw);
  }
  fprintf(stream , "\n");
}





int main(int argc , char ** argv) {
#define Nvar    4
#define var_len 5
#define N_well  20
  const int ens_size = 20;
  
  ecl_sum_type **prior;
  ecl_sum_type **posterior;
  
  const char *history_title     = "History";
  const int   history_pt        = 7;
  const char *prior_title       = "Prior";
  const char *posterior_title   = "Posterior";
  const int   prior_lt          = 3;
  const int   posterior_lt      = 1;
  const double lw               = 1.50;
  const double ps               = 0.75;
  
  const char *well_list[N_well] = {"PR03A_G8", "PR06_G28", "PR09_G10", "PR10_G18", "PR11E_A", "PR11E_G5", "PR11_G6", "PR12_G19", "PR13_G22", "PR14_G27", "PR15_G35", "PR18_G40", "PR22_G25", "PR23_G9", "PR24_G17", "PR25_G21", "PR26_G26", "PR27_G11", "PR29_G34", "T21A"}
  const char *well_list[N_well] = {"B-33A" , "B-37T2" , "B-43A","F-18A","F-24"};
  const char *var_list[Nvar]    = {"WOPT"  , "WOPR"  , "WGOR"   , "WWCT"};
  const char *hvar_list[Nvar]   = {"WOPTH" , "WOPRH" , "WGORH"  , "WWCTH"};
  const char *res_path       = "Plot";
  int i;
  char *posterior_path , *prior_path;
  
  if (argc != 3) {
    fprintf(stderr,"Usage:\n%s  prior_path posterior_path \n",argv[0]);
    exit(1);
  }
  prior_path = argv[1];
  posterior_path = argv[2];
  
  prior     = calloc(ens_size , sizeof (ecl_sum_type *));
  posterior = calloc(ens_size , sizeof (ecl_sum_type *));
  for (i = 0; i < ens_size; i++) {
    printf("Loading ens member: %3d: Prior...",i+1); fflush(stdout);
    prior[i]     = ecl_load(prior_path , "tmpdir_" , i );
    printf("  Posterior..."); fflush(stdout);
    posterior[i] = ecl_load(posterior_path , "tmpdir_" , i);
    printf("\n");
  }
  
  {
    int iw , ivar;
    for (iw = 0; iw < N_well; iw++) {
      FILE *gplot_stream;
      /*char gplot_file[128];*/

      for (ivar = 0; ivar < Nvar; ivar++) {
	FILE *data_stream ;
	int istep, iens;
	char data_file[128];
	sprintf(data_file  , "%s/%s-%s"       , res_path , well_list[iw] , var_list[ivar]);
	sprintf(gplot_file , "%s/%s-%s.gplot" , res_path , well_list[iw] , var_list[ivar]);
	data_stream = fopen(data_file , "w");
	
	for (istep = 0; istep < ecl_sum_get_size(prior[0]); istep++) {
	  float prior_value , posterior_value , history_value , time_value;
	  int index = ecl_sum_iget1(prior[0] , istep , well_list[iw] , var_list[ivar] , &prior_value);
	  ecl_sum_iget1(prior[0] , istep , well_list[iw] , hvar_list[ivar] , &history_value);

	  ecl_sum_iget2(prior[0] , istep , 0 , &time_value);
	  fprintf(data_stream , " %9.4e %9.4e " , time_value , history_value);
	  for (iens = 0; iens < ens_size; iens++) {
	    
	    ecl_sum_iget2(prior[iens]     , istep , index , &prior_value);
	    ecl_sum_iget2(posterior[iens] , istep , index , &posterior_value);
	    fprintf(data_stream , " %9.4e %9.4e " , prior_value , posterior_value);
	  }
	  fprintf(data_stream , "\n");
	}
	fclose(data_stream);

	/*
	  gplot_stream = fopen(gplot_file , "w");
	  fprintf(gplot_stream , "set term post enhanced color blacktext solid \"Helvetica\" 14\n");
	  fprintf(gplot_stream , "set output \"%s-%s.ps\"\n" , well_list[iw] , var_list[ivar]);
	  
	
	  add_subplot(gplot_stream , ens_size , ps , lw , history_pt , prior_lt , posterior_lt,
	  history_title , prior_title , posterior_title , well_list[iw] , var_list[ivar]);


	fprintf(gplot_stream , "!convert %s-%s.ps %s-%s.pdf" , well_list[iw] , var_list[ivar] , well_list[iw] , var_list[ivar]);
	fclose(gplot_stream);
      }
      

      sprintf(gplot_file , "%s/%s.gplot4" , res_path , well_list[iw]);
      gplot_stream = fopen(gplot_file , "w");
      fprintf(gplot_stream , "set term post enhanced color blacktext solid \"Hevetica\" 14 \n");
      fprintf(gplot_stream , "set output \"%s.ps\" \n" , well_list[iw]);
      fprintf(gplot_stream , "set size 1,1\n");
      fprintf(gplot_stream , "set origin 0,0\n");
      fprintf(gplot_stream , "set multiplot\n");
      fprintf(gplot_stream , "W = 0.49\n");
      fprintf(gplot_stream , "H = 0.49\n");
      fprintf(gplot_stream , "set size W,H\n");
      
      fprintf(gplot_stream , "set origin 0, 0.50\n");
      add_subplot(gplot_stream , ens_size , ps , lw , history_pt , prior_lt , posterior_lt,
		  history_title , prior_title , posterior_title , well_list[iw] ,var_list[0]);
      fprintf(gplot_stream , "\n\n");
      
      fprintf(gplot_stream , "set origin 0.50, 0.50\n");
      add_subplot(gplot_stream , ens_size , ps , lw , history_pt , prior_lt , posterior_lt,
		  history_title , prior_title , posterior_title , well_list[iw] ,var_list[1]);
      fprintf(gplot_stream , "\n\n");

      fprintf(gplot_stream , "set origin 0, 0.0\n");
      add_subplot(gplot_stream , ens_size , ps , lw , history_pt , prior_lt , posterior_lt,
		  history_title , prior_title , posterior_title , well_list[iw] ,var_list[2]);
      fprintf(gplot_stream , "\n\n");

      fprintf(gplot_stream , "set origin 0.50, 0.0\n");
      add_subplot(gplot_stream , ens_size , ps , lw , history_pt , prior_lt , posterior_lt,
		  history_title , prior_title , posterior_title , well_list[iw] ,var_list[3]);
      fprintf(gplot_stream , "\n\n");
      
      fprintf(gplot_stream , "set nomultiplot\n");
      fprintf(gplot_stream , "!convert -rotate 90 %s.ps %s.png\n" , well_list[iw] , well_list[iw]);

      fclose(gplot_stream);
    }
  }
  return 0;
}


