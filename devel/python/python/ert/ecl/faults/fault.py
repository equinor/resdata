#  Copyright (C) 2014  Statoil ASA, Norway. 
#   
#  The file 'fault.py' is part of ERT - Ensemble based Reservoir Tool. 
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

from ert.ecl import EclTypeEnum
from ert.geo import Polyline , CPolyline , GeometryTools
from ert.util import stat 
from ert.util import Matrix

from .fault_line import FaultLine
from .fault_segments import FaultSegment , SegmentMap


class Layer(object):
    def __init__(self, grid , K):
        self.__grid = grid
        self.__K = K
        self.__fault_lines = []
        self.__segment_map = SegmentMap()
        self.__processed = False

        
    def addSegment(self , segment):
        self.__segment_map.addSegment( segment )
        self.__processed = False

    def __len__(self):
        self.processSegments()
        return len(self.__fault_lines)

    def __iter__(self):
        self.processSegments()
        return iter(self.__fault_lines)

    def __getitem__(self , index):
        self.processSegments()
        return self.__fault_lines[index]

    def getK(self):
        return self.__K


    def getNeighborCells(self):
        neighbor_cells = []
        for fl in self:
            neighbor_cells += fl.getNeighborCells()
        return neighbor_cells

    def getPolyline(self):
        polyline = CPolyline()
        for fl in self:
            polyline += fl.getPolyline()
        return polyline


    def getIJPolyline(self):
        """
        Will return a python list of (int,int) tuple.
        """
        polyline = []
        for fl in self:
            polyline += fl.getIJPolyline()
        return polyline


    def numLines(self):
        return len(self)


    # A fault can typically consist of several non connected fault
    # segments; right after reading the fault input these can be in a
    # complete mess:
    #
    #  1. The different part of the fault can be in random order.
    #
    #  2. Within each fault line the micro segments can be ordered in
    #     reverse.
    #
    # This method goes through some desparate heuristics trying to
    # sort things out.

    def __sortFaultLines(self):
        N = len(self.__fault_lines)
        x = Matrix(N , 1)
        y = Matrix(N , 1)

        for index,line in enumerate(self.__fault_lines):
            xc,yc = line.center()
            
            x[index,0] = xc
            y[index,0] = yc

        # y = beta[0] + beta[1] * x
        #   = a       + b * x
        beta = stat.polyfit(2 , x , y)
        a = beta[0]
        b = beta[1]

        perm_list = []
        for index,line in enumerate(self.__fault_lines):
            x0 , y0 = line.center()
            d = x0 + b*(y0 - a)
            perm_list.append((index , d))
        perm_list.sort(key = lambda x: x[1])

        fault_lines = []
        for (index,d) in perm_list:
            fault_lines.append( self.__fault_lines[ index  ])
        self.__fault_lines = fault_lines    

        
        for line in self.__fault_lines:
            x1,y1 = line.startPoint()
            x2,y2 = line.endPoint()
            d1 = x1 + b*(y1 - a)
            d2 = x2 + b*(y2 - a)

            if d1 > d2:
                line.reverse()
                
        

        
    def processSegments(self):
        if self.__processed:
            return

        while self.__segment_map:
            fault_line = FaultLine(self.__grid , self.__K)
            self.__fault_lines.append( fault_line )

            current_segment = self.__segment_map.popStart()
            while current_segment:
                append = fault_line.tryAppend(current_segment)
                if not append:
                    fault_line = FaultLine(self.__grid , self.__K)
                    self.__fault_lines.append( fault_line )
                    fault_line.tryAppend(current_segment)

                current_segment.next_segment = self.__segment_map.popNext( current_segment )
                current_segment = current_segment.next_segment

        if len(self.__fault_lines) > 1:
            self.__sortFaultLines()

        self.__processed = True


#################################################################


class Fault(object):
    allowed_faces = ["X","Y","Z","I","J","K","X-","Y-","Z-","I-","J-","K-"]

    def __init__(self, grid , name):
        self.__grid = grid
        self.__name = name
        self.__layer_map  = {}
        self.__layer_list = []
        (self.nx , self.ny , self.nz , nactive) = grid.dims
        

    def __str__(self):
        return "Fault:%s" % self.__name

    def __getitem__(self , K):
        layer = self.__layer_map[K]
        return layer

    def __len__(self):
        return len(self.__layer_map)


    def __iter__(self):
        for layer in self.__layer_list:
            yield layer


    def hasLayer(self , K):
        return self.__layer_map.has_key( K )


    def addLayer(self , K):
        layer = Layer(self.__grid , K)
        self.__layer_map[K] = layer
        self.__layer_list.append( layer )


    def createSegment(self , I1 , I2 , J1 , J2 , face):
        if face in ["X" , "I"]:
            C1 = I1 + 1 + J1*(self.nx + 1)
            C2 = C1 + (1 + J2 - J1) * (self.nx + 1)
        elif face in ["X-" , "I-"]:
            C1 = I1 + J1*(self.nx + 1)
            C2 = C1 + (1 + J2 - J1) * (self.nx + 1)
        elif face in ["Y" , "J"]:
            C1 = I1 + (J1 + 1) * (self.nx + 1)
            C2 = C1 + (1 + I2 - I1)
        elif face in ["Y-" , "J-"]:
            C1 = I1 + J1 * (self.nx + 1)
            C2 = C1 + (1 + I2 - I1)
        else:
            raise Exception("Can only handle X,Y faces")

        return FaultSegment(C1,C2)
         

        
    def addRecord(self , I1 , I2 , J1 , J2 , K1 , K2 , face):
        if not face in Fault.allowed_faces:
            raise ValueError("Invalid face:%s" % face)
            
        if I1 > I2:
            raise ValueError("Invalid I1 I2 indices")

        if J1 > J2:
            raise ValueError("Invalid J1 J2 indices")

        if K1 > K2:
            raise ValueError("Invalid K1 K2 indices")
            
        if I1 < 0 or I1 >= self.nx:
            raise ValueError("Invalid I1:%d" % I1)
        if I2 < 0 or I2 >= self.nx:
            raise ValueError("Invalid I2:%d" % I2)

        if J1 < 0 or J1 >= self.ny:
            raise ValueError("Invalid J1:%d" % J1)
        if J2 < 0 or J2 >= self.ny:
            raise ValueError("Invalid J2:%d" % J2)

        if K1 < 0 or K1 >= self.nz:
            raise ValueError("Invalid K1:%d" % K1)
        if K2 < 0 or K2 >= self.nz:
            raise ValueError("Invalid K2:%d" % K2)

        if face in ["X","I"]:
            if I1 != I2:
                raise ValueError("For face:%s we must have I1 == I2" % face)

        if face in ["Y","J"]:
            if J1 != J2:
                raise ValueError("For face:%s we must have J1 == J2" % face)

        if face in ["Z","K"]:
            if K1 != K2:
                raise ValueError("For face:%s we must have K1 == K2" % face)
        
        #-----------------------------------------------------------------
        
        for K in range(K1,K2+1):
            if not self.hasLayer(K):
                self.addLayer(K)
            layer = self.__layer_map[K]
            segment = self.createSegment(I1,I2,J1,J2,face)
            layer.addSegment( segment )
            

    def getName(self):
        return self.__name


    def getNeighborCells(self):
        neighbor_cells = []
        for layer in self:
            neighbor_cells += layer.getNeighborCells()
        return neighbor_cells

        
    def getPolyline(self , k):
        layer = self[k]
        return layer.getPolyline()
        

    def getIJPolyline(self , k):
        layer = self[k]
        return layer.getIJPolyline()
        

    def numLines( self , k):
        layer = self[k]
        return layer.numLines()
    

    def extendToFault(self , other_fault , k):
        polyline = self.getIJPolyline(k)

        p0 = polyline[-2]
        p1 = polyline[-1]
        ray_dir = GeometryTools.lineToRay( p0 , p1 )
        intersections = GeometryTools.rayPolygonIntersections( p1 , ray_dir , other_fault.getIJPolyline(k))
        if intersections:
            p2 = intersections[0][1]
            return [p1 , (int(p2[0]) , int(p2[1])) ]
            
        p0 = polyline[1]
        p1 = polyline[0]
        ray_dir = GeometryTools.lineToRay( p0 , p1 )
        intersections = GeometryTools.rayPolygonIntersections( p1 , ray_dir , other_fault.getIJPolyline(k))
        if intersections:
            p2 = intersections[0][1]
            return [p1 , (int(p2[0]) , int(p2[1])) ]
            
        raise ValueError("The fault %s can not be extended to intersect with:%s" % (self.getName() , other_fault.getName()))


    def extendToPolyline(self , polyline , k):
        if self.intersectsPolyline(polyline , k):
            return None
            
        fault_polyline = self.getPolyline( k )
        p0 = fault_polyline[-2]
        p1 = fault_polyline[-1]
        ray_dir = GeometryTools.lineToRay( p0 , p1 )
        intersections = GeometryTools.rayPolygonIntersections( p1 , ray_dir , polyline)
        if intersections:
            p2 = intersections[0][1]
            return [(p1[0] , p1[1]) , p2]
            
        p0 = fault_polyline[1]
        p1 = fault_polyline[0]
        ray_dir = GeometryTools.lineToRay( p0 , p1 )
        intersections = GeometryTools.rayPolygonIntersections( p1 , ray_dir , polyline)
        if intersections:
            p2 = intersections[0][1]
            return [(p1[0] , p1[1]) , p2]

        raise ValueError("The fault %s can not be extended to intersect with polyline:%s" % (self.getName() , polyline.getName()))

        
    def intersectsPolyline(self , polyline , k):
        fault_line = self.getPolyline(k)
        return fault_line.intersects( polyline )


    def intersectsFault(self , other_fault , k):
        fault_line = other_fault.getPolyline(k)
        return self.intersectsPolyline( fault_line , k )


    def extendToFault(self , fault , k):
        fault_line = fault.getPolyline(k)
        return self.extendToPolyline(fault_line , k)

    def extendToEdge(self, edge , k):
        if isinstance(edge , Fault):
            return self.extendToFault( edge , k )
        else:
            return self.extendToPolyline( edge , k )
            

    def extendToBBox(self , bbox , k , start = True):
        fault_polyline = self.getPolyline(k)
        if start:
            p0 = fault_polyline[1]
            p1 = fault_polyline[0]
        else:
            p0 = fault_polyline[-2]
            p1 = fault_polyline[-1]
            
        ray_dir = GeometryTools.lineToRay(p0,p1)
        intersections = GeometryTools.rayPolygonIntersections( p1 , ray_dir , bbox)
        if intersections:
            p2 = intersections[0][1]
            if self.getName():
                name = "Extend:%s" % self.getName()
            else:
                name = None

            return CPolyline( name = name , init_points = [(p1[0] , p1[1]) , p2])
        else:
            raise Exception("Logical error - must intersect with bounding box")
            
    
        
            



    @staticmethod
    def intersectFaultRays(ray1 , ray2):
        p1,dir1 = ray1
        p2,dir2 = ray2
        if p1 == p2:
            return []
            
        dx = p2[0] - p1[0]
        dy = p2[1] - p1[1]
        if dx != 0:
            if dir1[0] * dx <= 0 and dir2[0] * dx >= 0:
                raise ValueError("Rays will never intersect")

        if dy != 0:
            if dir1[1] * dy <= 0 and dir2[1] * dy >= 0:
                raise ValueError("Rays will never intersect")

        if dx*dy != 0:
            if dir1[0] != 0:
                xc = p2[0]
                yc = p1[1]
            else:
                xc = p1[0]
                yc = p2[1]
                
            return [p1 , (xc,yc) , p2]
        else:
            return [p1,p2]


    @staticmethod
    def intRay(p1,p2):
        if p1 == p2:
            raise Exception("Can not form ray from coincident points")

        if p1[0] == p2[0]:
            # Vertical line
            dx = 0
            if p2[1] > p1[1]:
                dy = 1
            elif p2[1] < p1[1]:
                dy = -1
        else:
            # Horizontal line
            if p2[1] != p1[1]:
                raise Exception("Invalid direction")
                
            dy = 0
            if p2[0] > p1[0]:
                dx = 1
            else:
                dx = -1

        return [p2 , (dx,dy)]


    
    def getEndRays(self , k):
        polyline = self.getIJPolyline(k)
        
        p0 = polyline[0]
        p1 = polyline[1]
        p2 = polyline[-2]
        p3 = polyline[-1]

        return (Fault.intRay(p1,p0) , Fault.intRay(p2,p3))

                
        
    @classmethod
    def joinFaults(cls , fault1 , fault2 , k):
        fault1_rays = fault1.getEndRays(k)
        fault2_rays = fault2.getEndRays(k)
        
        count = 0
        join = None
        try:
            join = Fault.intersectFaultRays( fault1_rays[0] , fault2_rays[0] )
            count += 1
        except ValueError:
            pass

        try:
            join = Fault.intersectFaultRays( fault1_rays[0] , fault2_rays[1] )
            count += 1
        except ValueError:
            pass

        try:
            join = Fault.intersectFaultRays( fault1_rays[1] , fault2_rays[0] )
            count += 1
        except ValueError:
            pass

        try:
            join = Fault.intersectFaultRays( fault1_rays[1] , fault2_rays[1] )
            count += 1
        except ValueError:
            pass

        if count == 1:
            return join
        else:
            raise ValueError("Faults %s and %s could not be uniquely joined" % (fault1.getName() , fault2.getName()))
            
            
    
