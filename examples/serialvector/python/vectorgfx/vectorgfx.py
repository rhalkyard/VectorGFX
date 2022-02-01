import itertools
import pygame
import sys
import time


class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __eq__(self, other):
        return self.x == other.x and self.y == other.y

    def __hash__(self):
        return hash((self.x, self.y))

    def translate(self, x, y):
        return Point(self.x + x, self.y + y)

    def scale(self, x_factor, y_factor):
        return Point(self.x * x_factor, self.y * y_factor)


class Line:
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
        return "Line(({}, {}), ({}, {}))".format(self.p1.x, self.p1.y,
                                                 self.p2.x, self.p2.y)

    def translate(self, x, y):
        return Line(self.p1.translate(x, y), self.p2.translate(x, y))

    def scale(self, x, y):
        return Line(self.p1.scale(x, y), self.p2.scale(x, y))

    def max_x(self):
        return max(self.p1.x, self.p2.x)


def toPoint(x, y, bright, flag=2):
    return (flag << 30 | bright << 24 |
            (int(x) & 0xfff) << 12 | (int(y) & 0xfff)).to_bytes(4, 'big')


class VectorGraphics:
    def __init__(self, port, win_size=512):
        self.port = port
        self.screen = pygame.display.set_mode((win_size, win_size))
        self.screen.fill((0, 0, 0))
        pygame.font.init()
        self.font = pygame.font.Font(pygame.font.get_default_font(), 12)
        pygame.display.flip()
        self.scale_factor = win_size / 4096

    def sync(self):
        # Sending > 4 zero bytes in a row causes protcol parser to reset to
        # initial state
        if self.port is not None:
            self.port.write(bytes([0, 0, 0, 0]))

    def draw(self, lines, allow_duplicates=False):
        draw_start = time.time()
        last_x = None
        last_y = None

        if not allow_duplicates:
            lines = list(dict.fromkeys(lines))

        points = []

        self.screen.fill((0, 0, 0))
        for line in lines:
            if int(line.p1.x) == int(line.p2.x) and \
                    int(line.p1.y) == int(line.p2.y):
                # Skip any zero-length lines
                continue

            if line.p1.x != last_x or line.p1.y != last_y:
                # Insert a transit move if line's start point is not the end
                # point of the previous line
                points.append(toPoint(line.p1.x, line.p1.y, 0))

            pygame.draw.line(self.screen, (0, 255, 0),
                                (line.p1.x * self.scale_factor,
                                (4095-line.p1.y) * self.scale_factor),
                                (line.p2.x * self.scale_factor,
                                (4095-line.p2.y) * self.scale_factor))

            points.append(toPoint(line.p2.x, line.p2.y, 24))

            last_x = line.p2.x
            last_y = line.p2.y
        draw_end = time.time()
        tx_start = draw_end

        data = (bytes([0, 0, 0, 0]) +
                bytes(itertools.chain.from_iterable(points)) +
                bytes([1, 0, 0, 0]))
        if self.port is not None:
            self.port.write(data)
        else:
            # Roughly simulate time required to transmit vector data
            nbytes = len(data)
            time.sleep(nbytes/(115200/8))

        tx_end = time.time()

        draw_time = draw_end - draw_start
        tx_time = tx_end - tx_start
        total_time = tx_end - draw_start

        try:
            fps = 1/total_time
        except ZeroDivisionError:
            fps = -1

        text_surface = self.font.render(
            '{} lines, {} bytes @ {:.2f} fps ({:.2f} ms draw, '
            '{:.2f} ms tx, {:.2f} ms total)'.format(len(lines), len(data),
                                                    fps, draw_time*1000,
                                                    tx_time * 1000,
                                                    total_time * 1000),
            False, (255, 255, 255))
        self.screen.blit(text_surface, (0, 0))
        pygame.display.flip()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                sys.exit(0)
