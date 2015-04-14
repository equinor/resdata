/*
  This file is included from the ecl_kw.c file during compilation. It
  contains functions which are not really related to the ecl_kw as a
  datastructure, but rather use an ecl_kw instance in a function.
*/


/*
  This is an extremely special-case function written for the region
  creation code. Given a completed ecl_kw regions keyword, the purpose
  of this function is to "detect and correct" uninitialized cells with
  value 0. The cells we detect are assigned the value of the
  neighburs.
*/


void ecl_kw_fix_uninitialized(ecl_kw_type * ecl_kw , int nx , int ny , int nz) {
  int i,j,k;
  int * data = ecl_kw_get_ptr( ecl_kw );
  for (k=0; k < nz; k++)  {
    for (j=1; j < (ny - 1); j++) {
      int g0 = j * nx + k* nx*ny;
      int prev = data[ g0 ];
      int current = data[ g0 + 1 ];
      for (i=1; i < (nx - 1); i++) {
        int next = data[g0 + i + 1];

        if (current == 0) {
          if ((next == prev) && current != next) {
            if ((data[g0 + i + nx] == next) && (data[g0 + i - nx] == next))
              data[g0 + i] = next;
          }
        }

        prev = current;
        current = next;
      }
    }
  }
}
