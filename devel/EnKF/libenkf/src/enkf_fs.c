#include <stdlib.h>
#include <util.h>
#include <enkf_fs.h>
#include <path_fmt.h>
#include <enkf_node.h>

struct enkf_fs_struct {
  
};


enkf_fs_type * enkf_fs_alloc(void) {
  enkf_fs_type * fs = malloc(sizeof * fs);
  return fs;
}


