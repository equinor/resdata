#include <stdlib.h>
#include <stdio.h>
#include <ecl_sum.h>
#include <ecl_fstate.h>

ecl_sum_type * ecl_load(const char *path, int i) {
  char spec_file[128];
  char data_file[128];

  sprintf(spec_file , "%s/%04d/ECLIPSE.SMSPEC" , path , i+1);
  sprintf(data_file , "%s/%04d/ECLIPSE.UNSMRY" , path , i+1);
  return ecl_sum_load_unified(spec_file , data_file , ECL_FMT_AUTO , true);
}


int main(int argc , char ** argv) {
#define Nvar    3
#define var_len 5
#define N_well  5
  const int ens_size = 16;
  
  ecl_sum_type **prior;
  ecl_sum_type **posterior;
  
  const char *history_title     = "History";
  const char *history_pt        = "7";
  const char *prior_title       = "Prior";
  const char *posterior_title   = "Posterior";
  const char *prior_lt          = "3";
  const char *posterior_lt      = "1";
  const double lw               = 1.50;
  const double ps               = 0.75;

  const char *well_list[N_well] = {"B-33A" , "B-37T2" , "B-43A","F-18A","F-24"};
  const char *var_list[Nvar]    = {"WOPR"  , "WGOR"   , "WWCT"};
  const char *hvar_list[Nvar]   = {"WOPRH" , "WGORH"  , "WWCTH"};
  const char *prior_path     = "Prior";
  const char *posterior_path = "Posterior";
  const char *res_path       = "Plot";
  int i;
  
  prior     = calloc(ens_size , sizeof (ecl_sum_type *));
  posterior = calloc(ens_size , sizeof (ecl_sum_type *));
  for (i = 0; i < ens_size; i++) {
    printf("Loading ens member: %3d: Prior...",i+1); fflush(stdout);
    
    prior[i]     = ecl_load(prior_path , i );
    printf("  Posterior..."); fflush(stdout);
    posterior[i] = ecl_load(posterior_path , i);
    printf("\n");
  }
  {
    int iw , ivar;
    for (iw = 0; iw < N_well; iw++) {
      for (ivar = 0; ivar < Nvar; ivar++) {
	FILE *data_stream , *gplot_stream;
	int istep, iens;
	char data_file[128];
	char gplot_file[128];
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

	gplot_stream = fopen(gplot_file , "w");
	fprintf(gplot_stream , "set term post enhanced color blacktext solid \"Helvetica\" 14\n");
	fprintf(gplot_stream , "set output \"%s-%s.ps\"\n" , well_list[iw] , var_list[ivar]);
	fprintf(gplot_stream,"set pointsize %g\n" , ps);
	fprintf(gplot_stream,"set xlabel \"Time (days)\n");
	fprintf(gplot_stream,"set title \"%s %s\" \n",well_list[iw] , var_list[ivar]);
	fprintf(gplot_stream,"plot \"%s-%s\" u 1:2 t \"%s\" w p pt %s ,\\\n" , well_list[iw] , var_list[ivar] , history_title , history_pt);
	for (iens=0; iens < ens_size; iens++) {
	  if (iens == 0)
	    fprintf(gplot_stream , "\"\" u 1:%d t \"%s\" w l lt %s lw %g , \"\" u 1:%d t \"%s\" w l lt %s lw %g , \\\n",
		    iens*2+3 , prior_title , prior_lt , lw , iens*2+4 , posterior_title , posterior_lt , lw);
	  else if (iens == (ens_size - 1)) 
	    fprintf(gplot_stream , "\"\" u 1:%d t \"\" w l lt %s lw %g , \"\" u 1:%d t \"\" w l lt %s lw %g \n",
		    iens*2+3 , prior_lt , lw , iens*2 + 4 , posterior_lt , lw);
	  else
	    fprintf(gplot_stream , "\"\" u 1:%d t \"\" w l lt %s lw %g , \"\" u 1:%d t \"\" w l lt %s lw %g , \\\n",
		    iens*2+3 , prior_lt , lw , iens*2 + 4 , posterior_lt , lw);
	}
	fprintf(gplot_stream , "\n");
	fprintf(gplot_stream , "!convert %s-%s.ps %s-%s.pdf" , well_list[iw] , var_list[ivar] , well_list[iw] , var_list[ivar]);
	fclose(gplot_stream);
      }
    }
  }
  return 0;
}


