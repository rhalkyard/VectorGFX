import argparse
import serial
import sys
import time

from HersheyFonts import HersheyFonts
from vectorgfx import Point, Line, VectorGraphics


def get_extents(lines):
    return max(line.max_x() for line in lines)


def str2lines(font, text, xpos, ypos):
    return [Line(Point(x1, y1), Point(x2, y2)).translate(xpos, ypos)
            for (x1, y1), (x2, y2) in font.lines_for_text(text)]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--list-fonts', '-l', action='store_true',
                        help='List built-in font names and exit')
    parser.add_argument('--port', '-p',
                        help='Serial port for V.st/ESP32 device')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baud rate for V.st/ESP32 device')
    parser.add_argument('--window', '-w', action='store_true',
                        help='Display vectors in Pygame window')
    parser.add_argument('--size', '-s', type=int,
                        default=500, help='Font size')
    parser.add_argument('--xpos', '-x', type=int,
                        default=0, help='Starting X position')
    parser.add_argument('--ypos', '-y', type=int,
                        default=0, help='Starting Y position')
    fontgroup = parser.add_mutually_exclusive_group()
    fontgroup.add_argument('--font', '-f', default=None,
                           help='Load built-in font (use -l to list '
                           'available fonts')
    fontgroup.add_argument('--font-file', '-F',
                           help='Load font from JHF file')
    parser.add_argument('text', nargs='*')

    args = parser.parse_args()

    if args.list_fonts:
        print(HersheyFonts().default_font_names)
        sys.exit(0)
    if not args.port:
        parser.error('A serial port must be specified with --port')
    if not args.text:
        parser.error('No text!')

    font = HersheyFonts()
    if args.font_file:
        font.load_font_file(args.font_file)
    else:
        font.load_default_font(args.font)
    font.normalize_rendering(args.size)

    text = ' '.join(args.text)

    with serial.Serial(args.port, args.baud) as port:
        if args.window:
            v = VectorGraphics(port, win_size=512)
        else:
            v = VectorGraphics(port)

        lines = []

        y = 4095-(args.size)

        currentLine = ''
        lines = []

        for word in text.split():
            if currentLine:
                testLine = currentLine + ' ' + word
            else:
                testLine = word
            if get_extents(str2lines(font, testLine, 0, y)) < 4095:
                currentLine = testLine
            else:
                lines.extend(str2lines(font, currentLine, 0, y))
                currentLine = word
                y -= args.size

        lines.extend(str2lines(font, currentLine, 0, y))

        lines = [line.translate(args.xpos, -args.ypos) for line in lines]

        tstart = time.time()
        v.draw(lines)
        tend = time.time()

        print("{} lines transmitted in {:.3f} ms".format(
            len(lines), (tend - tstart) * 1000))

        if args.window:
            while True:
                v.draw(lines)
