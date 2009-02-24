#include <ecl_grid.h>
#include <ecl_util.h>
#include <fortio.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <util.h>
#include <string.h>
#include <int_vector.h>
#include <double_vector.h>



void extract_contact(const ecl_kw_type * lower_kw , const ecl_kw_type * upper_kw , const ecl_grid_type * ecl_grid , int_vector_type * ilist , int_vector_type * jlist , double_vector_type * contact) {
  int k,nz,active_size,nx,ny;
  double * column;

  ecl_grid_get_dims( ecl_grid , &nx, &ny , &nz , &active_size);
  column = util_malloc( nz * sizeof * column , __func__);
  for (k=0; k < nz; k++)
    column[k] = 1.0;
  
  free( column );
}



int main (int argc , char ** argv) {
  const char * grid_file    = argv[1];
  const char * restart_file = argv[2];
  char * mode               = argv[3];
  const char * index_file   = argv[4];
  int_vector_type * ilist   = int_vector_alloc(100 , -1);
  int_vector_type * jlist   = int_vector_alloc(100 , -1);
  double_vector_type * contact = double_vector_alloc(100 , 0.0);

  int nx,ny,nz,active_size;
  bool fmt_file;
  ecl_grid_type * grid = ecl_grid_alloc( grid_file , true );
  ecl_kw_type * upper_kw;
  ecl_kw_type * lower_kw;
  ecl_kw_type * soil = NULL;
  ecl_kw_type * sgas = NULL;
  ecl_kw_type * swat = NULL;
  

  ecl_grid_get_dims( grid , &nx , &ny , &nz , &active_size );
  ecl_util_get_file_type( restart_file , NULL , &fmt_file , NULL);
  {
    fortio_type * fortio = fortio_fopen(restart_file , "r" , true , fmt_file);
    if (ecl_kw_fseek_kw("SWAT" , true , false , fortio))
      swat = ecl_kw_fread_alloc(fortio);

    if (ecl_kw_fseek_kw("SGAS" , true , false , fortio))
      sgas = ecl_kw_fread_alloc(fortio);
    
    fortio_fclose( fortio );
  }

  soil = ecl_kw_alloc_scalar( "SOIL" , active_size , ecl_float_type , 1.0);
  if (swat != NULL)
    ecl_kw_inplace_sub(soil , swat);
  if (sgas != NULL)
    ecl_kw_inplace_sub(soil , sgas); 

    util_strupr( mode );
  if (strcmp( mode , "OWC") == 0) {
    upper_kw = soil;
    lower_kw = swat;
  } else if (strcmp( mode , "GOC") == 0) {
    upper_kw = sgas;
    lower_kw = soil;
  } else if (strcmp(mode , "GWC") == 0) {
    upper_kw = sgas;
    lower_kw = swat;
  } else {
    fprintf(stderr,"Mode: %s not recognized.\n\n" , mode);
    fprintf(stderr,"Valid modes are: \n");
    fprintf(stderr,"   OWC: Oil Water Contact \n");
    fprintf(stderr,"   GOC: Gas Oil   Contact \n");
    fprintf(stderr,"   GWC: Gas Water Contact \n");
    exit(1);
  }

  {
    FILE * stream = util_fopen(index_file , "r");
    int read_count;
    do {
      int i,j;
      read_count = fscanf(stream , "%d %d" , &i , &j);
      if (read_count == 2) {
	int_vector_append(ilist , i);
	int_vector_append(jlist , j);
      }
    } while (read_count == 2);
    fclose(stream);
  }

  extract_contact( lower_kw , upper_kw , grid , ilist , jlist , contact);
  if (sgas != NULL) ecl_kw_free( sgas );
  if (soil != NULL) ecl_kw_free( soil );
  if (swat != NULL) ecl_kw_free( swat );
  ecl_grid_free( grid );
}
