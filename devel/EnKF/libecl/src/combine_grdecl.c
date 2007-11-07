#include <stdlib.h>
#include <ecl_kw.h>
#include <ecl_util.h>
#include <stdio.h>
#include <stdbool.h>
#include <util.h>


int main(int argc, char ** argv) {

  int nx,ny,nz;
  char filename[128];
  ecl_type_enum ecl_type = ecl_float_type;
  bool endian_flip = true;

  ecl_kw_type  * main_kw;
  ecl_box_type * ecl_box;

  printf("Main filename => "); 
  scanf("%s" , filename); 
  printf("Total size of grid => ");
  scanf("%d %d %d" , &nx , &ny , &nz);
  
  {
    FILE * stream = util_fopen(filename , "r");
    main_kw = ecl_kw_fscanf_alloc_grdecl_data(stream , nx*ny*nz , ecl_type , endian_flip);
    fclose(stream);
  }
  ecl_box = ecl_box_alloc(nx,ny,nz,0,0,0,0,0,0);
  
  {
    int scan_count;
    int x1,x2,y1,y2,z1,z2;
    do {
      printf("Box: file x1 x2 y1 y2 z1 z2 (^D to exit): ");
      scan_count = scanf("%s %d %d %d %d %d %d" , filename , &x1 , &x2 , &y1 , &y2 , &z1 , &z2);
      if (scan_count == 7) {
	ecl_box_set_size(ecl_box, x1,x2 , y1 , y2 , z1 , z2);
	{
	  FILE * stream = util_fopen(filename , "r");
	  ecl_kw_type * sub_kw = ecl_kw_fscanf_alloc_grdecl_data(stream , ecl_box_get_box_size(ecl_box) , ecl_type , endian_flip);
	  ecl_kw_boxed_set(main_kw , sub_kw , ecl_box);
	  ecl_kw_free(sub_kw);
	  fclose(stream);
	}
      } else if (scan_count == 1) {
	FILE * stream = util_fopen(filename , "w");
	ecl_kw_fprintf_grdecl(main_kw , stream);
	printf("New kw saved to: %s \n",filename);
	fclose(stream);
      }
    } while (scan_count == 7);
  }

  exit(1);
}
