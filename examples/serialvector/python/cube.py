import argparse
import serial
import time
import vectorgfx

from vectorgfx import Point3D, Line3D
from HersheyFonts import HersheyFonts


class Cube:
    def __init__(self):

        self.vertices = [
            Point3D(-1, 1, -1),
            Point3D(1, 1, -1),
            Point3D(1, -1, -1),
            Point3D(-1, -1, -1),
            Point3D(-1, 1, 1),
            Point3D(1, 1, 1),
            Point3D(1, -1, 1),
            Point3D(-1, -1, 1)
        ]

        # Define the vertices that compose each of the 6 faces. These numbers
        # are indices to the vertices list defined above.
        self.faces = [(0, 1, 2, 3), (1, 5, 6, 2), (5, 4, 7, 6),
                      (4, 0, 3, 7), (0, 4, 5, 1), (3, 2, 6, 7)]

        self.angleX, self.angleY, self.angleZ = 0, 0, 0

        self.font = HersheyFonts()
        self.font.load_default_font('cursive')
        self.font.normalize_rendering(0.35)

    def run(self, vector):
        """ Main Loop """
        while 1:
            time.sleep(1/60)

            # Will hold transformed vertices.
            t = []
            gx = 0
            lines = []

            text = "Hello, world!"
            textlines = []
            for (x1, y1), (x2, y2) in self.font.lines_for_text(text):
                textlines.append(
                    Line3D(Point3D(x1, -y1), Point3D(x2, -y2)).translate(
                        0.1, 0.2, 0))

            for line in textlines:
                lines.append(line.translate(-1, 0, -1)
                             .rotateX(self.angleX)
                             .rotateY(self.angleY)
                             .rotateZ(self.angleZ)
                             .project(4096, 4096, 4096, 4))

            for v in self.vertices:
                # Rotate the point around X axis, then around Y axis, and
                # finally around Z axis.
                r = v.rotateX(self.angleX).rotateY(
                    self.angleY).rotateZ(self.angleZ)
                # Transform the point from 3D to 2D
                p = r.project(4096, 4096, 4096, 4)
                # Put the point in the list of transformed vertices
                t.append(p)

            for f in self.faces:
                lines.append(
                    Line3D(Point3D(t[f[0]].x, t[f[0]].y),
                           Point3D(t[f[1]].x, t[f[1]].y)))
                lines.append(
                    Line3D(Point3D(t[f[1]].x, t[f[1]].y),
                           Point3D(t[f[2]].x, t[f[2]].y)))
                lines.append(
                    Line3D(Point3D(t[f[2]].x, t[f[2]].y),
                           Point3D(t[f[3]].x, t[f[3]].y)))
                lines.append(
                    Line3D(Point3D(t[f[3]].x, t[f[3]].y),
                           Point3D(t[f[0]].x, t[f[0]].y)))

                gx += 1

            vector.draw(lines)

            self.angleX += 1
            self.angleY += 1
            self.angleZ += 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('port', help='Serial port for V.st/ESP32 device')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baud rate for V.st/ESP32 device')
    parser.add_argument('--window', '-w', action='store_true',
                        help='Display vectors in Pygame window')
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud) as port:

        if args.window:
            v = vectorgfx.VectorGraphics(port, win_size=512)
        else:
            v = vectorgfx.VectorGraphics(port)
        Cube().run(v)
