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
