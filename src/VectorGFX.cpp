#include "vectorgfx.h"

#include <driver/i2s.h>
#include <string.h>

void displayTask(void *gfxInstance)
{
    while (1)
    {
        ((VectorGFX *)gfxInstance)->doUpdate();
        vTaskDelay(1);
    }
}

VectorGFX::VectorGFX()
{
    this->bufferMutex = xSemaphoreCreateMutex();
}

VectorGFX::~VectorGFX()
{
    this->end();
    xSemaphoreTake(this->bufferMutex, portMAX_DELAY);
    vSemaphoreDelete(this->bufferMutex);
}

void VectorGFX::doUpdate()
{
    xSemaphoreTake(this->bufferMutex, portMAX_DELAY);
    const Vertex *buffer = this->frontBuffer;
    const size_t count = this->frontBufferCount;

    for (int n = 0; n < count; n++)
    {
        if (buffer[n].bright)
        {
            this->dacLineTo(buffer[n].x, buffer[n].y);
        }
        else
        {
            this->dacMoveTo(buffer[n].x, buffer[n].y);
        }
    }
    xSemaphoreGive(this->bufferMutex);

    /* Ensure that all points are flushed to the DAC */
    this->dacFlush();
}

void VectorGFX::dacFlush()
{
    /* Flush buffered samples to the DMA engine */
    size_t bytes_written;
    size_t bytes_to_write = this->dacBufferCount * sizeof(uint32_t);
    if (this->dacBufferCount > 0)
    {
        i2s_write(I2S_NUM_0, this->dacBuffer, bytes_to_write, &bytes_written, portMAX_DELAY);
        this->dacBufferCount = 0;
    }
}

void VectorGFX::dacWrite(uint16_t ch1, uint16_t ch2)
{
    /* i2s_write() is kinda expensive, so buffer up a batch of writes */
    if (this->dacBufferCount >= SAMPLE_BUF_SZ)
    {
        this->dacFlush();
    }

    this->dacBuffer[this->dacBufferCount++] = (ch2 << 16) | ch1;
}

void VectorGFX::dacMoveTo(uint16_t x, uint16_t y)
{
    this->x_pos = x;
    this->y_pos = y;

    /* The ESP32 DACs (and our coordinate system) are 12-bit, but the I2S engine
    assumes a 16-bit sample, discarding the low 4 bits. Thus we need to expand
    our 12 bit values to '16 bit' */

    this->dacWrite(x << 4, y << 4);
}

void VectorGFX::dacLineTo(uint16_t x1, uint16_t y1)
{
    int dx, dy, err, sx, sy;
    uint16_t x0 = this->x_pos;
    uint16_t y0 = this->y_pos;

    if (x0 <= x1)
    {
        dx = x1 - x0;
        sx = 1;
    }
    else
    {
        dx = x0 - x1;
        sx = -1;
    }

    if (y0 <= y1)
    {
        dy = y1 - y0;
        sy = 1;
    }
    else
    {
        dy = y0 - y1;
        sy = -1;
    }

    err = dx - dy;

    while (1)
    {
        if (x0 == x1 && y0 == y1)
        {
            break;
        }
        int e2 = 2 * err;

        if (e2 > -dy)
        {
            err = err - dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err = err + dx;
            y0 += sy;
        }
        this->dacMoveTo(x0, y0);
    }
}

void VectorGFX::begin(int taskPriority)
{
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = 1000000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)0, // default format
        .intr_alloc_flags = 0,                        // default interrupt priority
        .dma_buf_count = DAC_BUF_COUNT,
        .dma_buf_len = DAC_BUF_SZ,
        .use_apll = false, // can't use APLL with built-in DAC (TRM page 312)
        .tx_desc_auto_clear = UNDERRUN_ZERO};

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    i2s_set_pin(I2S_NUM_0, NULL); // use internal DAC
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);

    /*
    The I2S driver's frequency-divider math gives bad results for sample rates
    outside the typical audio range (> ~100kHz). To get around this, we manually
    poke our own values into the frequency divider registers after I2S is
    initialized.

    Page 304 of the Technical Reference Manual gives the following formulae:

    f_i2s = 160000000 / (I2S_CLKM_DIV_NUM + I2S_CLKM_DIV_B / I2S_CLKM_DIV_A)

    f_bck = f_i2s / I2S_TX_BCK_DIV_NUM

    For the case of the built-in DAC, samplerate = f_bck / n_channels (usually
    with I2S, this would also be divided by bits-per-channel, but for the
    internal DAC, the data path is parallel and f_bck appears to drive the word
    clock - see Page 314 of the TRM)

    Thus: 160MHz / (20 + 0/1) / 2 / 2 = 2 MHz.

    2MHz seems to be the sweet spot; at 2.5MHz, the CPU can't keep up with
    line-drawing. If you want to poke around at these yourself, the constraints
    are:

    1 >= I2S_CLKM_DIV_A > 64 

    0 >= I2S_CLKM_DIV_B > 64 

    2 >= I2S_CLKM_DIV_NUM_V > 256

    2 >= I2S_TX_BCK_DIV_NUM_V > 256

    The Technical Reference Manual recommends (p. 304) recommends that the
    fractional divisor (I2S_CLKM_DIV_B / I2S_CLKM_DIV_A) not be used, as it will
    introduce jitter.
    */

    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_A_V, 1, I2S_CLKM_DIV_A_S);
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_B_V, 0, I2S_CLKM_DIV_B_S);
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_NUM_V, 20, I2S_CLKM_DIV_NUM_S);
    SET_PERI_REG_BITS(I2S_SAMPLE_RATE_CONF_REG(0), I2S_TX_BCK_DIV_NUM_V, 2, I2S_TX_BCK_DIV_NUM_S);

    xTaskCreate(displayTask, "vectorgfx_display", 1000, this, taskPriority, &this->task);
}

void VectorGFX::end()
{
    xSemaphoreTake(this->bufferMutex, portMAX_DELAY);
    vTaskDelete(this->task);
    xSemaphoreGive(this->bufferMutex);
    i2s_driver_uninstall(I2S_NUM_0);
}

void VectorGFX::addVertex(Vertex v)
{
    if (this->backBufferCount < MAX_PTS)
    {
        this->backBuffer[this->backBufferCount++] = v;
    }
}

void VectorGFX::addVertex(uint16_t x, uint16_t y, uint8_t bright)
{
    Vertex v = {
        .x = x,
        .y = y,
        .bright = bright};

    this->addVertex(v);
}

void VectorGFX::addVertices(const Vertex *vertices, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        this->addVertex(vertices[i].x, vertices[i].y, vertices[i].bright);
    }
}

void VectorGFX::moveto(uint16_t x, uint16_t y)
{
    this->addVertex(x, y, 0);
}

void VectorGFX::lineto(uint16_t x, uint16_t y)
{
    this->addVertex(x, y, 255);
}

void VectorGFX::display()
{
    Vertex *tmp;

    /* Fill buffer as much as possible by repeating the vertex list. This helps
    prevent DMA underrun if the vertex list is short */
    for (int i = this->backBufferCount; i + this->backBufferCount < MAX_PTS; i += this->backBufferCount)
    {
        memcpy(this->backBuffer + i, this->backBuffer, this->backBufferCount);
    }

    xSemaphoreTake(this->bufferMutex, portMAX_DELAY);

    tmp = this->frontBuffer;
    this->frontBuffer = this->backBuffer;
    this->backBuffer = tmp;

    this->frontBufferCount = this->backBufferCount;
    this->backBufferCount = 0;
    xSemaphoreGive(this->bufferMutex);
}

Vertex VectorGFX::getLastVertex()
{
    return this->backBuffer[this->backBufferCount - 1];
}
