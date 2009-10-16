#include <util.h>
#include <stdlib.h>
#include <point.h>
#include <tetrahedron.h>





void tetrahedron_set_shared( tetrahedron_type * tet , point_type * p0 , point_type * p1 , point_type * p2 , point_type * p3) {
  tet->point_owner = false;
  tet->p0 = p0 ;
  tet->p1 = p1 ;
  tet->p2 = p2 ;
  tet->p3 = p3 ;
}


tetrahedron_type * tetrahedron_alloc( point_type * p0 , point_type * p1 , point_type * p2 , point_type * p3 , bool point_owner) {
  tetrahedron_type * tet = util_malloc( sizeof * tet , __func__);
  
  
  if (point_owner) {
    tet->point_owner = true;
    tet->p0 = point_copyc( p0 );
    tet->p1 = point_copyc( p1 );
    tet->p2 = point_copyc( p2 );
    tet->p3 = point_copyc( p3 );
  } else
    tetrahedron_set_shared( tet , p0 , p1 , p2 , p3 );
  
  return tet;
}



void tetrahedron_free( tetrahedron_type * tet ) {
  if (tet->point_owner) {
    point_free(tet->p0);
    point_free(tet->p1);
    point_free(tet->p2);
    point_free(tet->p3);
  } 
  free( tet );
}




/**
        |a·(b x c)| 
  V  =  -----------
            6   


  Wherea a,b and c are assumed to have origo as reference.
*/          


double tetrahedron_volume( const tetrahedron_type * tet ) {
  point_type a; 
  point_type b;
  point_type c;
  point_type b_x_c;

  point_copy_values(&a , tet->p0);
  point_copy_values(&b , tet->p1);
  point_copy_values(&c , tet->p2);
  
  point_inplace_sub(&a , tet->p3);
  point_inplace_sub(&b , tet->p3);
  point_inplace_sub(&c , tet->p3);

  point_vector_cross( &b_x_c , &b , &c);
  
  printf("%g \n",abs( point_dot_product( &a , &b_x_c) ) / 6.0);
  return abs( point_dot_product( &a , &b_x_c) ) / 6.0;
}



static int __sign( double x) {
  if (x >= 0) 
    return 1;
  else
    return -1;
}



bool tetrahedron_contains( const tetrahedron_type * tet , const point_type * p) {
  int    current_sign;
  double d0 , d1 , d2 , d3;
  point_type n;

  point_normal_vector( &n , tet->p0 , tet->p1 , tet->p2 );
  d3 = point_plane_distance( p , &n , tet->p0 );
  current_sign = __sign( d3 );

  point_normal_vector( &n , tet->p1 , tet->p2 , tet->p3);
  d0 = point_plane_distance( p , &n , tet->p1 );
  if (current_sign != __sign( d0 )) return false;
  

  point_normal_vector( &n , tet->p2 , tet->p3 , tet->p0);
  d1 = point_plane_distance( p , &n , tet->p2 );
  if (current_sign != __sign( d1 )) return false;
  
  point_normal_vector( &n , tet->p3 , tet->p0 , tet->p1);
  d2 = point_plane_distance( p , &n , tet->p3 );
  if (current_sign != __sign( d2 )) return false;
  
  return true;
}
