#ifndef __ECL_BOX_H__
#define __ECL_BOX_H__


typedef struct ecl_box_struct ecl_box_type;


void           ecl_box_set_size       (ecl_box_type * , int , int , int , int , int , int );
ecl_box_type * ecl_box_alloc          (int , int , int , int , int , int , int , int , int );
void 	       ecl_box_free            (ecl_box_type * );
void 	       ecl_box_set_values(const ecl_box_type * , char * , const char * , int );
int  	       ecl_box_get_total_size(const ecl_box_type * );
int  	       ecl_box_get_box_size(const ecl_box_type * );
void           ecl_box_set_limits(const ecl_box_type * , int *, int * , int * , int * , int * , int *);


#endif
