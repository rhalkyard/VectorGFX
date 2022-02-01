/*
Simple program to receive lists of vertices over the serial port and display
them.

Protocol decoder is based on that used by http://trmm.net/v.st, and should be
more-or-less coompatible. Data is sent as 32-bit words, MSB-first, as follows:

  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
| flag |    brightness   |             Y position            |             X position            |
|2 bits|     6 bits      |              12 bits              |              12 bits              |

The flag field is used as follows:
| Flag | Description                                  |
|------|----------------------------------------------|
| 00   | Sync word, reset command parser              |
| 01   | Terminator, send received vectors to display | 
| 10   | Vector data                                  |
| 11   | Unused                                       |

Due to the lack of a brightness/blanking output, the brightness field is only
used to differentiate between line-drawing and invisible 'transit' vectors. A 0
brightness value moves the cursor without drawing a line, a nonzero value draws
a line from the current cursor position to the given coordinates.

To reset the protocol parser, send 4 or more 0 bytes in a row.

An example 'frame' of vector data is shown below, drawing a square in the center
of the display

00 00 00 00   Sync word
80 20 02 00   Move cursor to (512, 512)
BF E0 02 00   Draw line to (512, 3584)
BF E0 0E 00   Draw line to (3584, 3584)
BF 20 0E 00   Draw line to (3584, 512)
BF 20 02 00   Draw line to (512, 512)
01 01 01 01   Terminator

After the terminator is sent, the received vectors will be displayed

*/

#include "VectorGFX.h"

VectorGFX gfx;

void testPattern()
{
    /* Square in bottom-left */
    gfx.moveto(0, 0);
    gfx.lineto(1024, 0);
    gfx.lineto(1024, 1024);
    gfx.lineto(0, 1024);
    gfx.lineto(0, 0);

    /* Fill square with starbust pattern */
    gfx.moveto(0, 0);
    for (unsigned j = 0; j <= 1024; j += 128)
    {

        gfx.lineto(1024, j);
        gfx.moveto(0, 0);
        gfx.lineto(j, 1024);
        gfx.moveto(0, 0);
    }
    for (unsigned j = 0; j < 1024; j += 128)
    {

        gfx.lineto(j, 1024);
        gfx.moveto(0, 0);
    }

    /* Triangle in bottom-right */
    gfx.moveto(4095, 0);
    gfx.lineto(4095 - 512, 0);
    gfx.lineto(4095 - 0, 512);
    gfx.lineto(4095, 0);

    /* Square in top-right */
    gfx.moveto(4095, 4095);
    gfx.lineto(4095 - 512, 4095);
    gfx.lineto(4095 - 512, 4095 - 512);
    gfx.lineto(4095, 4095 - 512);
    gfx.lineto(4095, 4095);

    /* Hourglass in top-left */
    gfx.moveto(0, 4095);
    gfx.lineto(512, 4095);
    gfx.lineto(0, 4095 - 512);
    gfx.lineto(512, 4095 - 512);
    gfx.lineto(0, 4095);

    /* Center target */
    gfx.moveto(2047, 2047 - 512);
    gfx.lineto(2047, 2047 + 512);
    gfx.moveto(2047 - 512, 2047);
    gfx.lineto(2047 + 512, 2047);
}

int read_data(int c)
{
    static uint32_t cmd;
    static unsigned offset;

    static unsigned rx_points;
    static unsigned do_resync;

    if (c < 0)
    {
        return -1;
    }

    // if we are resyncing, wait for a non-zero byte
    if (do_resync)
    {
        if (c == 0)
            return 0;
        do_resync = 0;
    }

    cmd = (cmd << 8) | c;
    offset++;

    if (offset != 4)
    {
        return 0;
    }

    // we have a new command
    // check for a resync
    if (cmd == 0)
    {
        do_resync = 1;
        offset = cmd = 0;
        return 0;
    }

    unsigned flag = (cmd >> 30) & 0x3;
    unsigned bright = (cmd >> 24) & 0x3F;
    unsigned x = (cmd >> 12) & 0xFFF;
    unsigned y = (cmd >> 0) & 0xFFF;

    offset = cmd = 0;
    rx_points++;
    // bright 0, switch buffers
    if (flag == 0)
    {
        // Attempt to detect and avoid blank screen condition
        if (rx_points < 2)
        {
            rx_points = 0;
            gfx.moveto(0, 0);
            gfx.lineto(0, 4095);
            gfx.lineto(4095, 4095);
            gfx.lineto(4095, 0);
            gfx.lineto(0, 0);
        }

        rx_points = 0;
        gfx.display();

        return 1;
    }
    rx_points++;

    Vertex v = {.x = x, .y = y, .bright = bright};

    gfx.addVertex(v);

    return 0;
}

void setup()
{
    Serial.begin(115200);

    gfx.begin();
    testPattern();
    gfx.display();
}

void loop()
{
    while (Serial.available())
    {
        read_data(Serial.read());
    }
}
