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
point_type * point_copyc( const point_type * p);
void         point_copy_values(point_type * p , const point_type * src);
void         point_vector_cross(point_type * A , const point_type * B , const point_type * C);
double       point_dot_product( const point_type * v1 , const point_type * v2);
void         point_normal_vector(point_type * n, const point_type * p0, const point_type * p1 , const point_type * p2);
double       point_plane_distance(const point_type * p , const point_type * n , const point_type * plane_point);

#endif
