#!/usr/bin/env python
#  Copyright (C) 2014  Statoil ASA, Norway. 
#   
#  The file 'test_grid.py' is part of ERT - Ensemble based Reservoir Tool.
#   
#  ERT is free software: you can redistribute it and/or modify 
#  it under the terms of the GNU General Public License as published by 
#  the Free Software Foundation, either version 3 of the License, or 
#  (at your option) any later version. 
#   
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#  FITNESS FOR A PARTICULAR PURPOSE.   
#   
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
#  for more details.
import os.path
from unittest import skipIf
import time
import itertools
from numpy import linspace

from ert.util import IntVector
from ert.ecl import EclGrid,EclKW,EclDataType, EclUnitTypeEnum, EclFile
from ert.ecl.faults import Layer , FaultCollection
from ert.test import ExtendedTestCase , TestAreaContext

# This dict is used to verify that corners are mapped to the correct
# cell with respect to containment.
CORNER_HOME = {
        (0, 0, 0) : 0,  (0, 0, 1) : 9,  (0, 0, 2) : 18, (0, 0, 3) : 18,
        (0, 1, 0) : 3,  (0, 1, 1) : 12, (0, 1, 2) : 21, (0, 1, 3) : 21,
        (0, 2, 0) : 6,  (0, 2, 1) : 15, (0, 2, 2) : 24, (0, 2, 3) : 24,
        (0, 3, 0) : 6,  (0, 3, 1) : 15, (0, 3, 2) : 24, (0, 3, 3) : 24,
        (1, 0, 0) : 1,  (1, 0, 1) : 10, (1, 0, 2) : 19, (1, 0, 3) : 19,
        (1, 1, 0) : 4,  (1, 1, 1) : 13, (1, 1, 2) : 22, (1, 1, 3) : 22,
        (1, 2, 0) : 7,  (1, 2, 1) : 16, (1, 2, 2) : 25, (1, 2, 3) : 25,
        (1, 3, 0) : 7,  (1, 3, 1) : 16, (1, 3, 2) : 25, (1, 3, 3) : 25,
        (2, 0, 0) : 2,  (2, 0, 1) : 11, (2, 0, 2) : 20, (2, 0, 3) : 20,
        (2, 1, 0) : 5,  (2, 1, 1) : 14, (2, 1, 2) : 23, (2, 1, 3) : 23,
        (2, 2, 0) : 8,  (2, 2, 1) : 17, (2, 2, 2) : 26, (2, 2, 3) : 26,
        (2, 3, 0) : 8,  (2, 3, 1) : 17, (2, 3, 2) : 26, (2, 3, 3) : 26,
        (3, 0, 0) : 2,  (3, 0, 1) : 11, (3, 0, 2) : 20, (3, 0, 3) : 20,
        (3, 1, 0) : 5,  (3, 1, 1) : 14, (3, 1, 2) : 23, (3, 1, 3) : 23,
        (3, 2, 0) : 8,  (3, 2, 1) : 17, (3, 2, 2) : 26, (3, 2, 3) : 26,
        (3, 3, 0) : 8,  (3, 3, 1) : 17, (3, 3, 2) : 26, (3, 3, 3) : 26
}

def createGridTestBase(dim, dV, offset=1):
    return [
            EclGrid.createRectangular(dim, dV),
            EclGrid.createWave(dim, dV, offset),
            EclGrid.createWave(dim, dV, offset, irregular=True),
            EclGrid.createWave(dim, dV, offset, concave=True),
            EclGrid.createWave(dim, dV, offset, irregular=True, concave=True),
            ]


# This test class should only have test cases which do not require
# external test data. Tests involving Statoil test data are in the
# test_grid_statoil module.
class GridTest(ExtendedTestCase):
    
    def test_oom_grid(self):
        nx = 2000
        ny = 2000
        nz = 2000

        with self.assertRaises(MemoryError):
            grid = EclGrid.createRectangular( (nx,ny,nz), (1,1,1))


    
    def test_posXYEdge(self):
        nx = 10
        ny = 11
        grid = EclGrid.createRectangular( (nx,ny,1) , (1,1,1) )
        self.assertEqual( grid.findCellCornerXY(0,0,0)  , 0 )
        self.assertEqual( grid.findCellCornerXY(nx,0,0) , nx)
        self.assertEqual( grid.findCellCornerXY(0 , ny , 0) , (nx + 1 ) * ny )
        self.assertEqual( grid.findCellCornerXY(nx,ny,0) , (nx + 1 ) * (ny + 1) - 1)
        
        self.assertEqual( grid.findCellCornerXY(0.25,0,0)  , 0 )
        self.assertEqual( grid.findCellCornerXY(0,0.25,0)  , 0 )
    
        self.assertEqual( grid.findCellCornerXY(nx - 0.25,0,0)  , nx )
        self.assertEqual( grid.findCellCornerXY(nx , 0.25,0)  , nx )
    
        self.assertEqual( grid.findCellCornerXY(0 , ny - 0.25, 0) , (nx + 1 ) * ny )
        self.assertEqual( grid.findCellCornerXY(0.25 , ny , 0) , (nx + 1 ) * ny )
    
        self.assertEqual( grid.findCellCornerXY(nx -0.25 ,ny,0) , (nx + 1 ) * (ny + 1) - 1)
        self.assertEqual( grid.findCellCornerXY(nx , ny - 0.25,0) , (nx + 1 ) * (ny + 1) - 1)
    
    
    def test_dims(self):
        grid = EclGrid.createRectangular( (10,20,30) , (1,1,1) )
        self.assertEqual( grid.getNX() , 10 )
        self.assertEqual( grid.getNY() , 20 )
        self.assertEqual( grid.getNZ() , 30 )
        self.assertEqual( grid.getGlobalSize() , 30*10*20 )
    
        self.assertEqual( grid.getDims() , (10,20,30,6000) )
        
    
    
    def test_create(self):
        with self.assertRaises(ValueError):
            grid = EclGrid.createRectangular( (10,20,30) , (1,1,1) , actnum = [0,1,1,2])
            
        with self.assertRaises(ValueError):
            grid = EclGrid.createRectangular( (10,20,30) , (1,1,1) , actnum = IntVector(initial_size = 10))
    
        actnum = IntVector(default_value = 1 , initial_size = 6000)
        actnum[0] = 0
        actnum[1] = 0
        grid = EclGrid.createRectangular( (10,20,30) , (1,1,1) , actnum = actnum)
        self.assertEqual( grid.getNumActive( ) , 30*20*10 - 2)

    def test_repr_and_name(self):
        grid = EclGrid.createRectangular((2,2,2), (10,10,10), actnum=[0,0,0,0,1,1,1,1])
        pfx = 'EclGrid('
        rep = repr(grid)
        self.assertEqual(pfx, rep[:len(pfx)])
        self.assertEqual(type(rep), type(''))
        self.assertEqual(type(grid.getName()), type(''))
        with TestAreaContext("python/ecl_grid/repr"):
            grid.save_EGRID("CASE.EGRID")
            g2 = EclGrid("CASE.EGRID")
            r2 = repr(g2)
            self.assertEqual(pfx, r2[:len(pfx)])
            self.assertEqual(type(r2), type(''))
            self.assertEqual(type(g2.getName()), type(''))

    def test_node_pos(self):
        grid = EclGrid.createRectangular( (10,20,30) , (1,1,1) )
        with self.assertRaises(IndexError):
            grid.getNodePos(-1,0,0)
    
        with self.assertRaises(IndexError):
            grid.getNodePos(11,0,0)
    
        p0 = grid.getNodePos(0,0,0)
        self.assertEqual( p0 , (0,0,0))
    
        p7 = grid.getNodePos(10,20,30)
        self.assertEqual( p7 , (10,20,30))
    
    
    def test_truncated_file(self):
        grid = EclGrid.createRectangular( (10,20,30) , (1,1,1) )
        with TestAreaContext("python/ecl_grid/truncated"):
            grid.save_EGRID( "TEST.EGRID")
    
            size = os.path.getsize( "TEST.EGRID")
            with open("TEST.EGRID" , "r+") as f:
                f.truncate( size / 2 )
    
            with self.assertRaises(IOError):
                EclGrid("TEST.EGRID")
    
    def test_posXY1(self):
        nx = 4
        ny = 1
        nz = 1
        grid = EclGrid.createRectangular( (nx,ny,nz) , (1,1,1) )
        (i,j) = grid.findCellXY( 0.5 , 0.5, 0 )   
        self.assertEqual(i , 0)
        self.assertEqual(j , 0)
    
        (i,j) = grid.findCellXY( 3.5 , 0.5, 0 )   
        self.assertEqual(i , 3)
        self.assertEqual(j , 0)
    
    
    def test_init_ACTNUM(self):
        nx = 10
        ny = 23
        nz = 7
        grid = EclGrid.createRectangular( (nx,ny,nz) , (1,1,1) )
        actnum = grid.exportACTNUM()
        
        self.assertEqual( len(actnum) , nx*ny*nz )
        self.assertEqual( actnum[0] , 1 )
        self.assertEqual( actnum[nx*ny*nz - 1] , 1 )
        
        actnum_kw = grid.exportACTNUMKw( )
        self.assertEqual(len(actnum_kw) , len(actnum))
        for a1,a2 in zip(actnum, actnum_kw):
            self.assertEqual(a1, a2)
    
    
    def test_posXY(self):
        nx = 10
        ny = 23
        nz = 7
        grid = EclGrid.createRectangular( (nx,ny,nz) , (1,1,1) )
        with self.assertRaises(IndexError):
            grid.findCellXY( 1 , 1, -1 )   
    
        with self.assertRaises(IndexError):
            grid.findCellXY( 1 , 1, nz + 1 )   
    
        with self.assertRaises(ValueError):
            grid.findCellXY(15 , 78 , 2)
        
            
        i,j = grid.findCellXY( 1.5 , 1.5 , 2 )
        self.assertEqual(i , 1)
        self.assertEqual(j , 1)
    
    
        for i in range(nx):
            for j in range(ny):
                p = grid.findCellXY(i + 0.5 , j+ 0.5 , 0)
                self.assertEqual( p[0] , i )
                self.assertEqual( p[1] , j )
        
        c = grid.findCellCornerXY( 0.10 , 0.10 , 0 )
        self.assertEqual(c , 0)
        
        c = grid.findCellCornerXY( 0.90 , 0.90 , 0 )
        self.assertEqual( c , (nx + 1) + 1 )
    
        c = grid.findCellCornerXY( 0.10 , 0.90 , 0 )
        self.assertEqual( c , (nx + 1) )
    
        c = grid.findCellCornerXY( 0.90 , 0.90 , 0 )
        self.assertEqual( c , (nx + 1) + 1 )
    
        c = grid.findCellCornerXY( 0.90 , 0.10 , 0 )
        self.assertEqual( c , 1 )
        
    def test_compressed_copy(self):
        nx = 10
        ny = 10
        nz = 10
        grid = EclGrid.createRectangular( (nx,ny,nz) , (1,1,1) )
        kw1 = EclKW("KW" , 1001 , EclDataType.ECL_INT )
        with self.assertRaises(ValueError):
            cp = grid.compressedKWCopy( kw1 )
    
    
    def test_dxdydz(self):
        nx = 10
        ny = 10
        nz = 10
        grid = EclGrid.createRectangular( (nx,ny,nz) , (2,3,4) )
    
        (dx,dy,dz) = grid.getCellDims( active_index = 0 )
        self.assertEqual( dx , 2 )
        self.assertEqual( dy , 3 )
        self.assertEqual( dz , 4 )
        
    def test_numpy3D(self):
        nx = 10
        ny = 7
        nz = 5
        grid = EclGrid.createRectangular((nx,ny,nz) , (1,1,1))
        kw = EclKW( "SWAT" , nx*ny*nz , EclDataType.ECL_FLOAT )
        numpy_3d = grid.create3D( kw )

    def test_len(self):
        nx = 10
        ny = 11
        nz = 12
        actnum = EclKW( "ACTNUM" , nx*ny*nz , EclDataType.ECL_INT )
        actnum[0] = 1
        actnum[1] = 1
        actnum[2] = 1
        actnum[3] = 1
        
        grid = EclGrid.createRectangular( (nx,ny,nz) , (1,1,1), actnum = actnum)
        self.assertEqual( len(grid) , nx*ny*nz )
        self.assertEqual( grid.getNumActive( ) , 4 )


    def test_output_units(self):
        n = 10
        a = 1
        grid = EclGrid.createRectangular( (n,n,n), (a,a,a))

        with TestAreaContext("python/ecl_grid/units"):
            grid.save_EGRID( "CASE.EGRID" , output_unit = EclUnitTypeEnum.ECL_FIELD_UNITS )
            f = EclFile("CASE.EGRID")
            g = f["GRIDUNIT"][0]
            self.assertEqual( g[0].strip( ) , "FEET" )
            g2 = EclGrid("CASE.EGRID")
            self.assertFloatEqual( g2.cell_volume( global_index = 0 ) , 3.28084*3.28084*3.28084)

            
            grid.save_EGRID( "CASE.EGRID" )
            f = EclFile("CASE.EGRID")
            g = f["GRIDUNIT"][0]
            self.assertEqual( g[0].strip( ) , "METRES" )
            
            grid.save_EGRID( "CASE.EGRID" , output_unit = EclUnitTypeEnum.ECL_LAB_UNITS)
            f = EclFile("CASE.EGRID")
            g = f["GRIDUNIT"][0]
            self.assertEqual( g[0].strip() , "CM" )
            g2 = EclGrid("CASE.EGRID")
            self.assertFloatEqual( g2.cell_volume( global_index = 0 ) , 100*100*100 )

    def test_cell_corner_containment(self):
        n = 4
        d = 10
        grid = EclGrid.createRectangular( (n, n, n), (d, d, d))

        for x, y, z in itertools.product(range(0, n*d+1, d), repeat=3):
            self.assertEqual(
                    1,
                    [grid.cell_contains(x, y, z, i) for i in range(n**3)].count(True)
                    )

    def test_cell_corner_containment_compatability(self):
        grid = EclGrid.createRectangular( (3,3,3), (1,1,1) )

        for x, y, z in itertools.product(range(4), repeat=3):
            for i in range(27):
                if grid.cell_contains(x, y, z, i):
                    self.assertEqual(
                            CORNER_HOME[(x,y,z)],
                            i
                            )

    def test_cell_face_containment(self):
        n = 4
        d = 10
        grid = EclGrid.createRectangular( (n, n, n), (d, d, d))

        for x, y, z in itertools.product(range(d/2, n*d, d), repeat=3):
            for axis, direction in itertools.product(range(3), [-1, 1]):
                p = [x, y, z]
                p[axis] = p[axis] + direction*d/2
                self.assertEqual(
                        1,
                        [grid.cell_contains(p[0], p[1], p[2], i) for i in range(n**3)].count(True)
                    )

    def test_cell_unique_containment(self):
        n = 4
        d = 4
        grid = EclGrid.createRectangular( (n, n, n), (d, d, d))

        coordinates = range(0, n*d+1)
        for x, y, z in itertools.product(coordinates, repeat=3):
            self.assertEqual(
                    1,
                    [grid.cell_contains(x, y, z, i) for i in range(n**3)].count(True)
                    )

    def test_volume(self):
        epsilon = 1e-10
        dim     = (10,10,10)
        dV      = (2,2,2)
        tot_vol = dim[0]*dV[0] * dim[1]*dV[1] * dim[2]*dV[2]

        grids = createGridTestBase(dim, dV)
        for grid in grids:
            cell_volumes = [grid.cell_volume(i) for i in range(grid.getGlobalSize())]
            self.assertTrue(min(cell_volumes) >= 0)
            self.assertTrue(abs(sum(cell_volumes) - tot_vol) < epsilon)
