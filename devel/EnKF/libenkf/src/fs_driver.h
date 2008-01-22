#ifndef __FS_DRIVER_H__
#define __FS_DRIVER_H__
#include <enkf_node.h>
#include <path_fmt.h>

typedef struct fs_driver_struct fs_driver_type;
typedef void (load_node_ftype) 	  (enkf_node_type * , const char *);
typedef void (save_node_ftype) 	  (enkf_node_type * , const char *);
typedef void (swapin_node_ftype)  (enkf_node_type * , const char *);
typedef void (swapout_node_ftype) (enkf_node_type * , const char *);


fs_driver_type * fs_driver_alloc(path_fmt_type *  , load_node_ftype * , save_node_ftype * , swapin_node_ftype * , swapout_node_ftype);
void             fs_driver_free(fs_driver_type *);


void fs_driver_save_node   (fs_driver_type *, int  , int , bool , enkf_node_type * );
void fs_driver_load_node   (fs_driver_type *, int  , int , bool , enkf_node_type * );
void fs_driver_swapin_node (fs_driver_type *, int  , int , bool , enkf_node_type * );
void fs_driver_swapout_node(fs_driver_type *, int  , int , bool , enkf_node_type * );

#endif
