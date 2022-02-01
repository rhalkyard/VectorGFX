#include <VectorGFX.h>

VectorGFX gfx;

void draw_box(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    gfx.moveto(x0, y0);
    gfx.lineto(x1, y0);
    gfx.lineto(x1, y1);
    gfx.lineto(x0, y1);
    gfx.lineto(x0, y0);
}

void draw_x(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    gfx.moveto(x0, y0);
    gfx.lineto(x1, y1);
    gfx.moveto(x0, y1);
    gfx.lineto(x1, y0);
}

void setup() {
    gfx.begin();
}

void loop() {
    // Draw a border around the edges of the screen
    draw_box(0, 0, 4095, 4095);
    gfx.display();  // Must call display() to flip buffers and display the image
    delay(1000);

    // Draw an X between the corners of the screen
    // Note that we start with a clean slate after the call to display() earlier
    draw_x(0, 0, 4095, 4095);
    gfx.display();
    delay(1000);
    
    // Draw a box and an X together
    draw_box(0, 0, 4095, 4095);
    draw_x(0, 0, 4095, 4095);
    gfx.display();
    delay(1000);
}
