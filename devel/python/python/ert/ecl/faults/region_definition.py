#  Copyright (C) 2014  Statoil ASA, Norway. 
#   
#  The file 'region_definition.py' is part of ERT - Ensemble based Reservoir Tool. 
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
from collections import OrderedDict

from ert.geo import Polyline , XYZIo, GeometryTools , CPolyline , CPolylineCollection
from ert.ecl.faults import Fault, FaultBlockLayer , Layer 
from ert.ecl import EclKW, EclTypeEnum

class RegionDefinition(object):

    def __init__(self , region_id):
        if not isinstance(region_id , int):
            raise TypeError("The region_id input argument must be integer. region_id:%s - invalid" % region_id)
            
        self.region_id = region_id
        self.edges = []
        self.__has_polygon = False
        self.__faults = OrderedDict()
        self.__polylines = CPolylineCollection( )
        self.extend_faults = []
        
            

    @staticmethod
    def create(region_id , faults , edge_names):
        regionDef = RegionDefinition( region_id )

        for edge_name in edge_names:
            if isinstance(edge_name , str):
                if faults.hasFault( edge_name ):
                    regionDef.addFault( faults[ edge_name ] )
                elif os.path.exists( edge_name ):
                    polyline = CPolyline.createFromXYZFile( edge_name )
                    regionDef.addPolyline( polyline )
                else:
                    raise ValueError("The elements in edge_list must be strings with either name of faults or filename with polygon. %s: invalid" % edge_name)
            else:
                raise ValueError("The elements in edge_list must be strings with either name of faults or filename with polygon. %s: invalid" % edge_name)

        return regionDef

    
    def getRegionID(self):
        return self.region_id


    def addFault(self , fault , extend_to = None):
        self.edges.append( fault )
        self.__faults[ fault.getName() ] = fault

        if fault.getName() == "DF29_MC":
            extend_to = self.__faults["DF27_LC"]
            
        if extend_to:
            self.extend_faults.append( (fault , extend_to) )
            


    def addPolyline(self , polyline):
        self.edges.append( polyline )
        self.__has_polygon = True
        self.__polylines.addPolyline( polyline )



    def polylines(self):
        return self.__polylines


    def faults(self):
        return self.__faults.values()


    def faultConnections(self):
        return self.extend_faults



    def __convexHull(self , k):
        point_list = []
        
        if len(self.edges) == 0:
            raise Exception("You must add Fault / Polyline edges first")
            
        for edge in self.edges:
            if isinstance(edge , Fault):
                fault = edge
                layer = fault[k]
                for line in layer:
                    for p in line.getPolyline():
                        point_list.append( (p[0] , p[1]) )
            else:
                poly_line = edge
                for p in poly_line:
                    point_list.append( (p[0] , p[1]) )
        
        return GeometryTools.convexHull( point_list )    
        
        

    def edgeCount(self):
        return len(self.edges)



    def hasPolygon(self):
        return self.__has_polygon


            
            
            



