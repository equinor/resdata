#ifndef __ECL_DIAG_H__
#define __ECL_DIAG_H__

#include <stdbool.h>

void ecl_diag_ens(int  , int , const char *  , int , const char **, int , const char **, const char *, const char *, const char * ,  bool , bool , bool , bool , bool );
/*void ecl_diag_ens(int , int  , const char ** , const char ** , const char * , const char * , const char * , bool , bool , bool , bool);*/
void ecl_diag_ens_interactive(const char* , const char * , bool , bool , bool);
void ecl_diag_avg_production_interactive(const char * , const char * , const char * , const char * , bool , bool );
/*
  void ecl_diag_make_gnuplot_interactive();
*/

#endif
