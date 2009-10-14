#ifndef __POINT_H__
#define __POINT_H__

#include <stdbool.h>
#include <stdio.h>

typedef struct point_struct  point_type;

struct point_struct {
  double x;
  double y;
  double z;
};


point_type * point_alloc_empty( );
void         point_inplace_sub(point_type * point , const point_type * sub);
void         point_inplace_add(point_type * point , const point_type * add);
void         point_inplace_scale(point_type * point , double scale_factor);
void         point_compare( const point_type *p1 , const point_type * p2, bool * equal);
void         point_fprintf( const point_type * p , FILE * stream );
void         point_free( point_type * p);
void         point_set( point_type *p , double x , double y , double z);

#endif
