#pragma once

#ifdef ARDUINO
#include <FreeRTOS.h>
#else
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#endif
#include <stdint.h>

// Max number of points to display
#define MAX_PTS 8192

// Number and size of DMA buffers
#define DAC_BUF_COUNT 8
#define DAC_BUF_SZ 1024

// Size of sample buffer
#define SAMPLE_BUF_SZ DAC_BUF_SZ

// DAC Sample rate (Hz)
#define SAMPLE_RATE 250000

// How to handle DMA buffer underrun
// true: output 0 value - this makes underrun conditions obvious as a bright dot at (0, 0)
// false: repeat contents of last DMA buffer - this helps mask underruns
#define UNDERRUN_ZERO true

extern "C"
{
    /**
     * @brief C-linked stub for display-update task
     * 
     * Repeatedly call the doUpdate() method of the given VectorGFX instance
     * 
     * @param gfxInstance Pointer to VectorGFX instance
     */
    void displayTask(void *gfxInstance);
}

typedef struct
{
    uint32_t x : 12;
    uint32_t y : 12;
    uint32_t bright : 8;
} Vertex;

class VectorGFX
{
public:
    VectorGFX();

    ~VectorGFX();

    /**
     * @brief Start vector display
     *
     * Set up DAC outouts and start display-update task
     *
     * @param samplerate DAC sample rate - higher rates mean less flicker, but
     * require more CPU time to maintain
     *
     * @param taskPriority Priority of background display-update task
     */
    void begin(int samplerate = 250000, int taskPriority = 24);

    /**
     * @brief Stop vector display
     *
     * Shuts down the DAC and I2S peripherals and stops display-update task.
     */
    void end();

    /**
     * @brief Append a vertex to the display buffer
     * 
     * @param vertex Vertex to add
     */
    void addVertex(const Vertex vertex);

    /**
     * @brief Append a vertex to the display buffer
     * 
     * @param x X position
     * @param y Y position
     * @param bright Brightness (0 to move cursor without drawing, nonzero to draw line)
     */
    void addVertex(uint16_t x, uint16_t y, uint8_t bright);

    /**
     * @brief Add multiple vertices to the display buffer
     * 
     * @param vertices Array of vertices
     * @param count Number of vertices to add
     */
    void addVertices(const Vertex *vertices, size_t count);

    /**
     * @brief Move to position (shorthand for addVertex(x, y, 0))
     * 
     * @param x X position
     * @param y Y position
     */
    void moveto(uint16_t x, uint16_t y);

    /**
     * @brief Draw line to position (shorthand for addVertex(x, y, 255))
     * 
     * @param x X position
     * @param y Y position
     */
    void lineto(uint16_t x, uint16_t y);

    /**
     * @brief Return the last vertex in the buffer
     * 
     * @return Vertex 
     */
    Vertex getLastVertex();

    /**
     * @brief Display the current buffer
     * 
     */
    void display();

private:
    friend void displayTask(void *gfxInstance);

    /**
     * @brief Render the contents of the vertex buffer to the DAC
     */
    void doUpdate();

    /**
     * @brief Generate a single point, moving the beam instantaneously
     * 
     * @param x 
     * @param y 
     */
    void dacMoveTo(uint16_t x, uint16_t y);

    /**
     * @brief Generate a sequence of points using Bresenham's algorithm,
     * sweeping the beam in a visible line.
     *
     * @param x 
     * @param y 
     */
    void dacLineTo(uint16_t x, uint16_t y);

    /**
     * @brief Write a single sample to the DAC buffer
     * 
     * Task may block if the DAC buffer is full.
     * 
     * @param ch1 
     * @param ch2 
     */
    void dacWrite(uint16_t ch1, uint16_t ch2);

    /**
     * @brief Flush the DAC buffer
     * 
     */
    void dacFlush();

    /**
     * @brief Handle for display-update task
     * 
     * Retained so that display task can be killed when end() is called.
     * 
     */
    TaskHandle_t task;

    /**
     * @brief Mutex controlling access to front/back buffer switching
     * 
     */
    SemaphoreHandle_t bufferMutex;

    /**
     * @brief Vertex buffers
     *
     * At any one point in time, one buffer is the 'front buffer', being read by
     * doUpdate() to render the display, while the other is the 'back buffer',
     * being written by addVertex().
     *
     * Calling display() flips the two buffers, using bufferMutex to wait until
     * doUpdate() is not accessing the buffer.
     *
     */
    Vertex buffer[2][MAX_PTS];

    /**
     * @brief Pointer to current front buffer
     * 
     */
    Vertex * volatile frontBuffer = buffer[0];

    /**
     * @brief Pointer to current back buffer
     * 
     */
    Vertex *backBuffer = buffer[1];

    /**
     * @brief Number of vertices in front buffer
     * 
     */
    size_t frontBufferCount = 0;

    /**
     * @brief Number of vertices in back buffer
     * 
     */
    size_t backBufferCount = 0;

    /**
     * @brief DAC sample buffer
     * 
     */
    uint32_t dacBuffer[SAMPLE_BUF_SZ];

    /**
     * @brief DAC sample buffer count
     * 
     */
    size_t dacBufferCount = 0;

    uint16_t x_pos;
    uint16_t y_pos;
};
