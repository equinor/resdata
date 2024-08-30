import os
from .polyline import Polyline


class XYZIo:
    @staticmethod
    def readXYZFile(path):
        """@rtype: Polyline"""

        if not os.path.exists(path):
            raise OSError(f"Path does not exist '{path}'!")

        name = os.path.basename(path)

        polyline = Polyline(name=name)

        with open(path) as f:
            for line in f:
                line = line.strip()
                if line:
                    x, y, z = map(float, line.split())

                    if x != 999.000000 and y != 999.000000 and z != 999.000000:
                        polyline.addPoint(x, y, z)
                    else:
                        break
                else:
                    break

        return polyline

    @staticmethod
    def readXYFile(path):
        """@rtype: Polyline"""

        if not os.path.exists(path):
            raise OSError(f"Path does not exist '{path}'!")

        name = os.path.basename(path)

        polyline = Polyline(name=name)

        with open(path) as f:
            for line in f:
                x, y = map(float, line.split())
                polyline.addPoint(x, y)

        return polyline

    @staticmethod
    def saveXYFile(polyline, filename):
        """
        @type polyline: Polyline or list of tuple of (float, float)
        """
        with open(filename, "w") as fileH:
            for p in polyline:
                fileH.write(f"{p[0]:g} {p[1]:g}\n")
