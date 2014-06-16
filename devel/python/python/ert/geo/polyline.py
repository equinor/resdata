class Polyline(object):
    def __init__(self, name="Unnamed"):
        super(Polyline, self).__init__()
        self.__name = name
        self.__points = []

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


    def __iter__(self):
        index = 0

        while index < len(self):
            yield self[index]
            index += 1
