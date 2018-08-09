#include <ert/util/test_work_area.hpp>
#include <ert/util/test_util.hpp>

#include <ert/ecl/ecl_grid.hpp>


void test_1() {
  //make a main grid (4 cells)
  //write it to file
  //allocate it w/ ext actnum
  //verify correct correct cells
  test_work_area_type * work_area = test_work_area_alloc("ext_actnum_main_grid");
  {
    const char * filename = "FILE.EGRID";

    ecl_grid_type * grid_write = ecl_grid_alloc_rectangular(2,2,1,   1,1,1,NULL);
    ecl_grid_fwrite_EGRID( grid_write , filename, true);
    ecl_grid_free(grid_write);

    int * ext_actnum = new int[4];
    ext_actnum[0] = 0;
    ext_actnum[1] = 1; 
    ext_actnum[2] = 0; 
    ext_actnum[3] = 1;

    ecl_grid_type * grid = ecl_grid_alloc_ext_actnum(filename, (const int*)ext_actnum);
    test_assert_int_equal( 2, ecl_grid_get_nactive(grid) );
    test_assert_true( !ecl_grid_cell_active1(grid, 0) );
    test_assert_true(  ecl_grid_cell_active1(grid, 1) );
    test_assert_true( !ecl_grid_cell_active1(grid, 2) );
    test_assert_true(  ecl_grid_cell_active1(grid, 3) );
    
    delete[] ext_actnum;
    ecl_grid_free( grid );

  }
  test_work_area_free( work_area );
  

}


int main() {
  test_1();
}
