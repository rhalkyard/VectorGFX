import math


class Point3D:
    def __init__(self, x=0, y=0, z=0):
        self.x, self.y, self.z = x, y, z

    def __eq__(self, other):
        return self.x == other.x and self.y == other.y and self.z == other.z

    def __hash__(self):
        return hash((self.x, self.y, self.z))

    def translate(self, x, y, z):
        return Point3D(self.x + x, self.y + y, self.z + z)

    def scale(self, x_factor, y_factor, z_factor):
        return Point3D(self.x * x_factor, self.y * y_factor, self.z * z_factor)

    def rotateX(self, angle):
        """ Rotates this point around the X axis the given number of degrees.
        """
        cosa = math.cos(math.radians(angle))
        sina = math.sin(math.radians(angle))
        y = self.y * cosa - self.z * sina
        z = self.y * sina + self.z * cosa
        return Point3D(self.x, y, z)

    def rotateY(self, angle):
        """ Rotates this point around the Y axis the given number of degrees.
        """
        cosa = math.cos(math.radians(angle))
        sina = math.sin(math.radians(angle))
        z = self.z * cosa - self.x * sina
        x = self.z * sina + self.x * cosa
        return Point3D(x, self.y, z)

    def rotateZ(self, angle):
        """ Rotates this point around the Z axis the given number of degrees.
        """
        cosa = math.cos(math.radians(angle))
        sina = math.sin(math.radians(angle))
        x = self.x * cosa - self.y * sina
        y = self.x * sina + self.y * cosa
        return Point3D(x, y, self.z)

    def project(self, win_width, win_height, fov, viewer_distance):
        """ Transforms this 3D point to 2D using a perspective projection. """
        factor = fov / (viewer_distance + self.z)
        x = self.x * factor + win_width / 2
        y = -self.y * factor + win_height / 2
        return Point3D(x, y, self.z)


class Line3D:
    def __init__(self, p1, p2):
        self.p1 = p1
        self.p2 = p2

    def __eq__(self, other):
        return ((self.p1 == other.p1 and self.p2 == other.p2) or
                (self.p1 == other.p2 and self.p2 == other.p1))

    def __hash__(self):
        if (self.p1.x < self.p2.x or
                (self.p1.x == self.p2.x and self.p1.y < self.p2.y)):
            return hash((self.p1, self.p2))
        else:
            return hash((self.p2, self.p1))

    def __repr__(self):
        return "Line3D(({}, {}), ({}, {}))".format(self.p1.x, self.p1.y,
                                                   self.p2.x, self.p2.y)

    def translate(self, x, y, z):
        return Line3D(self.p1.translate(x, y, z), self.p2.translate(x, y, z))

    def scale(self, x, y, z):
        return Line3D(self.p1.scale(x, y, z), self.p2.scale(x, y, z))

    def rotateX(self, angle):
        return Line3D(self.p1.rotateX(angle), self.p2.rotateX(angle))

    def rotateY(self, angle):
        return Line3D(self.p1.rotateY(angle), self.p2.rotateY(angle))

    def rotateZ(self, angle):
        return Line3D(self.p1.rotateZ(angle), self.p2.rotateZ(angle))

    def project(self, win_width, win_height, fov, viewer_distance):
        return Line3D(self.p1.project(win_width, win_height, fov,
                                      viewer_distance),
                      self.p2.project(win_width, win_height, fov,
                                      viewer_distance))

    def max_x(self):
        return max(self.p1.x, self.p2.x)


class Scene3D:
    def __init__(self, lines):
        self.lines = lines

    def translate(self, x, y, z):
        return Scene3D(*(line.translate(x, y, z) for line in self.lines))

    def rotateX(self, angle):
        return Scene3D(*(line.rotateX(angle) for line in self.lines))

    def rotateY(self, angle):
        return Scene3D(*(line.rotateY(angle) for line in self.lines))

    def rotateZ(self, angle):
        return Scene3D(*(line.rotateZ(angle) for line in self.lines))

    def project(self, win_width, win_height, fov, viewer_distance):
        return (line.project(self, win_width, win_height, fov, viewer_distance)
                for line in self.lines)
