class GeometryTools(object):
    EPSILON = 0.000001

    @staticmethod
    def lineIntersection(p1, p2, p3, p4):
        """
        Finds intersection between line segments. Returns None if no intersection found.
        Algorithm provided by Paul Bourke

        @type p1: tuple of (float, float)
        @type p2: tuple of (float, float)
        @type p3: tuple of (float, float)
        @type p4: tuple of (float, float)
        @rtype: tuple of (float, float)
        """

        denominator = (p4[1] - p3[1]) * (p2[0] - p1[0]) - (p4[0] - p3[0]) * (p2[1] - p1[1])
        numerator_a = (p4[0] - p3[0]) * (p1[1] - p3[1]) - (p4[1] - p3[1]) * (p1[0] - p3[0])
        numerator_b = (p2[0] - p1[0]) * (p1[1] - p3[1]) - (p2[1] - p1[1]) * (p1[0] - p3[0])

        # coincident?
        if abs(numerator_a) < GeometryTools.EPSILON and abs(numerator_b) < GeometryTools.EPSILON and abs(denominator) < GeometryTools.EPSILON:
            x = (p1[0] + p2[0]) / 2.0
            y = (p1[1] + p2[1]) / 2.0
            return x, y

        # parallel?
        if abs(denominator) < GeometryTools.EPSILON:
            return None


        # intersection along the segments?
        mua = numerator_a / denominator
        mub = numerator_b / denominator

        if mua < 0.0 or mua > 1.0 or mub < 0.0 or mub > 1.0:
            return None

        x = p1[0] + mua * (p2[0] - p1[0])
        y = p1[1] + mua * (p2[1] - p1[1])
        return x, y


    @staticmethod
    def ccw(p1, p2, p3):
        """
        Three points are a counter-clockwise turn if ccw > 0, clockwise if
        ccw < 0, and collinear if ccw = 0 because ccw is a determinant that
        gives the signed area of the triangle formed by p1, p2 and p3.

        @type p1: tuple of (float, float)
        @type p2: tuple of (float, float)
        @type p3: tuple of (float, float)
        @rtype: float
        """
        return (p2[0] - p1[0]) * (p3[1] - p1[1]) - (p2[1] - p1[1]) * (p3[0] - p1[0])


    @staticmethod
    def convexHull(points):
        """
        Given a list of points finds the convex hull
        @type points: list of tuple of (float, float)
        @rtype: list of tuple of (float, float)
        """
        points = sorted(points)

        def keepLeft(hull, r):
            while len(hull) > 1 and GeometryTools.ccw(hull[-2], hull[-1], r) > 0:
                hull.pop()

            if len(hull) == 0 or hull[-1] != r:
                hull.append(r)

            return hull

        l = reduce(keepLeft, points, [])
        u = reduce(keepLeft, reversed(points), [])
        l.extend([u[i] for i in xrange(1, len(u) - 1)])

        return l


    @staticmethod
    def pointInPolygon(p, polygon):
        """
        Finds out if a point is inside a polygon or not
        @type p: tuple of (float, float)
        @type polygon: Polyline or list of tuple of (float, float)
        @rtype: bool
        """
        x = p[0]
        y = p[1]
        n = len(polygon)

        inside = False

        p1x, p1y = polygon[0][0:2]
        for index in range(n + 1):
            p2x, p2y = polygon[index % n][0:2]

            if min(p1y, p2y) < y <= max(p1y, p2y):
                if x <= max(p1x, p2x):
                    if p1y != p2y:
                        xints = (y - p1y) * (p2x - p1x) / (p2y - p1y) + p1x

                    if p1x == p2x or x <= xints:
                        inside = not inside

            p1x, p1y = p2x, p2y

        return inside
