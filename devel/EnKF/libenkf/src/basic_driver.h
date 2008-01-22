#ifndef __BASIC_DRIVER_H__
#define __BASIC_DRIVER_H__
#include <enkf_node.h>

typedef struct basic_driver_struct basic_driver_type;


typedef void (load_node_ftype) 	  (void * , int , int , bool , enkf_node_type *);
typedef void (save_node_ftype) 	  (void * , int , int , bool , enkf_node_type *);
typedef void (swapin_node_ftype)  (void * , int , int , bool , enkf_node_type *);
typedef void (swapout_node_ftype) (void * , int , int , bool , enkf_node_type *);
typedef void (free_driver_ftype)  (void *);


#define BASIC_DRIVER_FIELDS   	   \
load_node_ftype    * load;    	   \
save_node_ftype    * save;    	   \
swapout_node_ftype * swapout; 	   \
swapin_node_ftype  * swapin;  	   \
free_driver_ftype  * free_driver;  \
int                  basic_type_id 


struct basic_driver_struct {
  BASIC_DRIVER_FIELDS;
};



void 	 basic_driver_init(basic_driver_type * );
void 	 basic_driver_assert_cast(const basic_driver_type * );


#endif
