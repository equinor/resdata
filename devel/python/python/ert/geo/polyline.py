import collections
from .geometry_tools import GeometryTools

class Polyline(object):
    def __init__(self, name="Unnamed" , init_points = None):
        super(Polyline, self).__init__()
        self.__name = name
        self.__points = []
        if init_points:
            self.__loadPoints( init_points )

    def name(self):
        """ @rtype: str """
        return self.__name

    def __len__(self):
        return len(self.__points)

    def addPoint(self, x, y, z=None):
        if z is None:
            p = (x, y)
        else:
            p = (x, y, z)
        self.__points.append(p)

    def __getitem__(self, index):
        """ @rtype: tuple of (float, float, float) """
        if not 0 <= index < len(self):
            raise IndexError("Index must be in range: [0, %d]" % (len(self) - 1))
        return self.__points[index]

    def isClosed(self):
        first = self[0]
        last = self[len(self) - 1]
        return first == last

    def __loadPoints(self , points):
        if not isinstance( points , collections.Iterable ):
            raise TypeError("The input argument points must be iterable")
            
        for point in points:
            x = point[0]
            y = point[1]
            try:
                z = point[2]
            except IndexError:
                z = None
            
            self.addPoint(x,y,z)
                

    def intersects(self, other_polyline):
        """
        Test if instances intersects with other polyline.

        @type other_polyline: Polyline or list of tuple of (float, float)
        @rtype: bool
        """
        return GeometryTools.polylinesIntersect( self , other_polyline )

    def __iter__(self):
        index = 0

        while index < len(self):
            yield self[index]
            index += 1
    def unzip(self):
        first_point = self[0]
        x = []
        y = []

        try:
            z = first_point[2]
            z = []
            for p in self:
                x.append(p[0])
                y.append(p[1])
                z.append(p[2])

            return (x,y,z)
        except IndexError:
            for p in self:
                x.append(p[0])
                y.append(p[1])
            
            return (x,y)
            
