#ifndef __TETRAHEDRON_H__
#define __TETRAHEDRON_H__


typedef struct tetrahedron_struct tetrahedron_type;

struct tetrahedron_struct {
  point_type * p0;
  point_type * p1;
  point_type * p2;
  point_type * p3; 
  bool point_owner;   /* Should the tetrahedron structure free the point instances when going down?? */
};



void               tetrahedron_set_shared( tetrahedron_type * tet , point_type * p0 , point_type * p1 , point_type * p2 , point_type * p3);
tetrahedron_type * tetrahedron_alloc( point_type * p0 , point_type * p1 , point_type * p2 , point_type * p3 , bool point_owner);
void               tetrahedron_free( tetrahedron_type * tetrahedron );
double             tetrahedron_volume( const tetrahedron_type * tet );
bool               tetrahedron_contains( const tetrahedron_type * tet , const point_type * p);
#endif


