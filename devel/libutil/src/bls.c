#include <util.h>
#include <block_fs.h>
#include <vector.h>


int main(int argc , char ** argv) {
  const char * mount_file      = argv[1];
  if (block_fs_is_mount(mount_file)) {
    block_fs_sort_type sort_mode = NO_SORT;
    const char * pattern         = NULL;
    int iarg;

    for (iarg = 2; iarg < argc; iarg++) {
      if (argv[iarg][0] == '-') {
        /** OK - this is an option .. */
      }
      else pattern = argv[iarg];
    }
    
    {
      block_fs_type * block_fs = block_fs_mount(mount_file , 1 , 0 , 1 , false , true );
      vector_type   * files    = block_fs_alloc_filelist( block_fs , pattern , sort_mode , false );
      {
        int i;
        for (i=0; i < vector_get_size( files ); i++) {
          const file_node_type * node = vector_iget_const( files , i );
          printf("%-40s   %d    \n",file_node_get_filename( node ), file_node_get_data_size( node ));
        }
      }
      vector_free( files );
      block_fs_close( block_fs , false );
    }
  } else 
    fprintf(stderr,"The file:%s does not seem to be a block_fs mount file.\n" , mount_file);
}
