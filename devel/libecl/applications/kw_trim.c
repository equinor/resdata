/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'kw_trim.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <ecl_kw.h>
#include <fortio.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_file.h>
#include <set.h>
#include <fortio.h>
#include <hash.h>
#include <stdbool.h>
#include <ecl_grid.h>
#include <ecl_endian_flip.h>


static void usage() {
  fprintf(stderr,"\n");
  fprintf(stderr,"This program is used to remove keywords from an ECLIPSE file\n");
  fprintf(stderr,"inplace. The program is used as follows:\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"bash%%  kw_trim FILENAME kw1 kw2 kw3\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"This will update the file FILENAME --IN-PLACE-- so only the\n");
  fprintf(stderr,"keywords kw1, kw2 and kw3 are kept. I.e. to only retain the\n");
  fprintf(stderr,"solution vectors in a restart file:\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"bash%% kw_trim RESTART_FILE PRESSURE SWAT SGAS RS RV\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Observe the following:\n");
  fprintf(stderr,"\n");
  fprintf(stderr," 1. You must name at least one keyword to keep - if you give no\n");
  fprintf(stderr,"    keyword arguments that amounts to removing the file - and that\n");
  fprintf(stderr,"    is done with 'rm'.\n");
  fprintf(stderr,"\n");
  fprintf(stderr," 2. If a keyword occurs several times in file all instances are\n");
  fprintf(stderr,"    kept.\n");
  fprintf(stderr,"\n");
  fprintf(stderr," 3. If a keyword given on the command line does not appear in the\n");
  fprintf(stderr,"    file that keyword is simply ignored; however if none of the\n");
  fprintf(stderr,"    keywords on the commandline are present that is considered\n");
  fprintf(stderr,"    illegal input, and no file update is performed.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"The resulting file will of course be smaller - however it might\n");
  fprintf(stderr,"very well be invalid as input to other programs - your call!.\n");
  fprintf(stderr,"\n");
  exit(1);
}


int main(int argc, char ** argv) {
  if (argc < 3) 
    usage();
  {
    const char *  src_file   = argv[1];
    const char ** kw_list  = (const char **) &argv[2];
    int num_kw = argc - 2;
    bool fmt_file     = ecl_util_fmt_file(src_file);
    set_type * kw_set = set_alloc( num_kw , kw_list );
    ecl_file_type * ecl_file = ecl_file_fread_alloc( src_file );
    fortio_type * fortio;
    int ikw;
    
    {
      bool has_kw = false;
      int ikw     = 0;
      while (!has_kw && (ikw < num_kw)) {
        has_kw = ecl_file_has_kw( ecl_file , kw_list[ ikw ]);
        ikw++;
      }

      if (!has_kw) {
        fprintf(stderr,"None of the keywords: ");
        for (ikw = 0; ikw  <num_kw; ikw++)
          fprintf(stderr,"\'%s\' ",kw_list[ikw]);
        fprintf(stderr," appeared in the file: %s - no update done \n", src_file );
        ecl_file_free( ecl_file );
        exit(1);
      }
    }
    
    {
      fortio  = fortio_fopen_writer(src_file , ECL_ENDIAN_FLIP, fmt_file );
      for (ikw = 0; ikw < ecl_file_get_num_kw( ecl_file ); ikw++) {
        const ecl_kw_type * ecl_kw = ecl_file_iget_kw( ecl_file , ikw );
        const char * kw = ecl_kw_get_header( ecl_kw );
        if (set_has_key( kw_set , kw))
          ecl_kw_fwrite(ecl_kw , fortio );
      }
      fortio_fclose( fortio );
    }
    ecl_file_free( ecl_file );
    set_free( kw_set );
  }
}
