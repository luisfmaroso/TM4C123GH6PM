#include "ssd1306.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Tiva headers */
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/i2c.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/pwm.h"
#include "inc/hw_gpio.h"

#define SSD1306_CMD  0x00
#define SSD1306_DATA 0x40

static uint32_t g_i2c_base = 0;
static uint8_t  g_i2c_addr = SSD1306_I2C_ADDR;

#if (SSD1306_WIDTH != 128)
#error "This driver currently expects 128 pixel width. Adjust code if needed."
#endif

static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

static const FontDef *current_font = &Font6x8;
static int16_t cursor_x = 0;
static int16_t cursor_y = 0;

// --- Low level I2C write helpers ---
static void i2cWrite(uint8_t controlByte, const uint8_t *data, uint32_t len)
{
    // controlByte = 0x00 for command, 0x40 for data
    // Many SSD1306 I2C displays expect the control byte as the first transmitted byte in the payload.
    // We'll send: [slave addr + WR] -> [controlByte] [payload...]
    // Tiva I2C supports multi-byte transfers via MasterDataPut and MasterControl.
    // Build temporary buffer in RAM if necessary:
    // But to avoid malloc: send controlByte then each byte.
    // Use single-byte writes (slower) but robust.
    // Use blocking waits for simplicity.
    uint32_t i;
    for (i = 0; i < len; ++i) {
        // For many modules, you must send control byte only once before a multi-byte stream.
        // We'll send control byte for the first byte only, then only data bytes.
        if (i == 0) {
            // Put first byte as combined control + data by starting transmit and then data
            // Use I2C Master in burst: first write controlByte, then data...
            // Simpler approach: send a small local array with controlByte + data chunk <= 16
            // But driverlib doesn't give a convenient multi-send without filling whole sequence.
        }
    }
    // Simpler and compatible way: create a small stack buffer to hold controlByte + data up to some size.
    // Many writes will be relatively small: commands or pages of 128 bytes. We will chunk.
    #define CHUNK 32
    uint8_t tmp[CHUNK + 1];
    uint32_t offset = 0;
    while (offset < len) {
        uint32_t chunk = len - offset;
        if (chunk > CHUNK) chunk = CHUNK;
        tmp[0] = controlByte;
        memcpy(&tmp[1], &data[offset], chunk);
        // send tmp[0..chunk] as one I2C transaction
        // Setup slave address, write mode
        MAP_I2CMasterSlaveAddrSet(g_i2c_base, g_i2c_addr, false);
        // Start transmission
        MAP_I2CMasterDataPut(g_i2c_base, tmp[0]); // first byte (control)
        MAP_I2CMasterControl(g_i2c_base, I2C_MASTER_CMD_BURST_SEND_START);
        while (MAP_I2CMasterBusy(g_i2c_base));
        uint32_t j = 0;
        for (j = 0; j < chunk; ++j) {
            MAP_I2CMasterDataPut(g_i2c_base, tmp[1 + j]);
            if (j == chunk - 1) {
                MAP_I2CMasterControl(g_i2c_base, I2C_MASTER_CMD_BURST_SEND_FINISH);
            } else {
                MAP_I2CMasterControl(g_i2c_base, I2C_MASTER_CMD_BURST_SEND_CONT);
            }
            while (MAP_I2CMasterBusy(g_i2c_base));
        }
        offset += chunk;
    }
    #undef CHUNK
}

static void i2cWriteCommand(uint8_t cmd)
{
    i2cWrite(SSD1306_CMD, &cmd, 1);
}

static void i2cWriteCommands(const uint8_t *cmds, uint32_t len)
{
    i2cWrite(SSD1306_CMD, cmds, len);
}

static void i2cWriteData(const uint8_t *data, uint32_t len)
{
    i2cWrite(SSD1306_DATA, data, len);
}

// --- SSD1306 commands ---
static void ssd1306_command_init(void)
{
    const uint8_t init_seq[] = {
        0xAE, // display off
        0xD5, 0x80, // set display clock divide ratio/oscillator frequency
        0xA8, (SSD1306_HEIGHT - 1), // multiplex
        0xD3, 0x00, // display offset
        0x40, // start line = 0
        0x8D, 0x14, // charge pump on
        0x20, 0x00, // memory addressing mode: horizontal addressing
        0xA1, // segment remap
        0xC8, // COM scan direction remapped
        0xDA, 0x12, // COM pins hardware config (0x02 for 32px?)
        0x81, 0xCF, // contrast
        0xD9, 0xF1, // pre-charge
        0xDB, 0x40, // vcom detect
        0xA4, // resume to RAM display
        0xA6, // normal display (A7 inverse)
        0x2E, // deactivate scroll
        0xAF // display ON
    };
    i2cWriteCommands(init_seq, sizeof(init_seq));
}

// --- buffer helpers ---
static inline void buffer_clear(void) {
    memset(buffer, 0x00, sizeof(buffer));
}

void SSD1306_Clear(void)
{
    buffer_clear();
    SSD1306_Display();
}

void SSD1306_Display(void)
{
    // Write buffer to display as pages (each page = 8 rows)
    const uint8_t set_col_cmds[] = {0x21, 0, SSD1306_WIDTH - 1}; // set column address
    const uint8_t set_page_cmds[] = {0x22, 0, (SSD1306_HEIGHT/8) - 1}; // set page address
    i2cWriteCommand(set_col_cmds[0]); // we'll send these commands separately to be safe
    i2cWriteCommand(set_col_cmds[1]);
    i2cWriteCommand(set_col_cmds[2]);
    i2cWriteCommand(set_page_cmds[0]);
    i2cWriteCommand(set_page_cmds[1]);
    i2cWriteCommand(set_page_cmds[2]);

    // Many displays prefer the "control byte 0x40 then 128*pages bytes"
    // Send entire buffer as data
    i2cWriteData(buffer, sizeof(buffer));
}

void SSD1306_Invert(bool invert)
{
    if (invert) i2cWriteCommand(0xA7);
    else i2cWriteCommand(0xA6);
}

void SSD1306_SetContrast(uint8_t contrast)
{
    const uint8_t cmd[] = {0x81, contrast};
    i2cWriteCommands(cmd, 2);
}

// --- Pixel & primitives ---
void SSD1306_DrawPixel(int16_t x, int16_t y, bool color)
{
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    uint16_t index = x + (y / 8) * SSD1306_WIDTH;
    if (color) buffer[index] |= (1 << (y & 7));
    else buffer[index] &= ~(1 << (y & 7));
}

void SSD1306_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color)
{
    // Bresenham
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;
    for (;;) {
        SSD1306_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void SSD1306_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color)
{
    SSD1306_DrawLine(x, y, x + w - 1, y, color);
    SSD1306_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    SSD1306_DrawLine(x, y, x, y + h - 1, color);
    SSD1306_DrawLine(x + w - 1, y, x + w - 1, y + h -1, color);
}

void SSD1306_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color)
{
    int16_t i = 0, j = 0;
    for (i = x; i < x + w; ++i) {
        for (j = y; j < y + h; ++j) {
            SSD1306_DrawPixel(i, j, color);
        }
    }
}

void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, bool color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    SSD1306_DrawPixel(x0, y0 + r, color);
    SSD1306_DrawPixel(x0, y0 - r, color);
    SSD1306_DrawPixel(x0 + r, y0, color);
    SSD1306_DrawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawPixel(x0 + x, y0 + y, color);
        SSD1306_DrawPixel(x0 - x, y0 + y, color);
        SSD1306_DrawPixel(x0 + x, y0 - y, color);
        SSD1306_DrawPixel(x0 - x, y0 - y, color);
        SSD1306_DrawPixel(x0 + y, y0 + x, color);
        SSD1306_DrawPixel(x0 - y, y0 + x, color);
        SSD1306_DrawPixel(x0 + y, y0 - x, color);
        SSD1306_DrawPixel(x0 - y, y0 - x, color);
    }
}

// Bitmaps: expects bitmap stored as bytes row-major or column-major depending on how you export.
// We'll assume a simple row-major with each byte representing 8 vertical pixels (like many SSD1306 exporters).
void SSD1306_DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, bool color)
{
    // For each column of the bitmap:
    int16_t i = 0, j = 0;
    for (j = 0; j < h; ++j) {
        for (i = 0; i < w; ++i) {
            // compute source bit
            int16_t byteIndex = (i + (j / 8) * w);
            uint8_t byte = bitmap[byteIndex];
            uint8_t bit = 1 << (j & 7);
            bool pixelOn = (byte & bit);
            if (pixelOn) SSD1306_DrawPixel(x + i, y + j, color);
            else SSD1306_DrawPixel(x + i, y + j, !color);
        }
    }
}

// --- Text ---
void SSD1306_SetCursor(int16_t x, int16_t y)
{
    // x in pixels, y in lines (rows of text), but we provide y as pixel lines for flexibility
    cursor_x = x;
    cursor_y = y;
}

void SSD1306_WriteString(const char *str)
{
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y += 8;
            ++str;
            continue;
        }
        SSD1306_WriteChar(*str++);
    }
}
static void float_to_string(float num, char *buffer, int precision);
void SSD1306_WriteInt(int32_t val)
{
    char buf[16] = {0};
    float_to_string(val, buf, 0);
    SSD1306_WriteString(buf);
}

void SSD1306_WriteFloat(float val, uint8_t decimals)
{
    char buf[32] = {0};
    float_to_string(val, buf, decimals);
    SSD1306_WriteString(buf);
}

// --- Initialization ---
void SSD1306_Init(uint32_t i2c_base, uint8_t i2c_addr)
{
    g_i2c_base = i2c_base;
    g_i2c_addr = i2c_addr;

    // Clear buffer
    buffer_clear();

    // Init sequence
    ssd1306_command_init();

    // default cursor
    cursor_x = 0;
    cursor_y = 0;
}




// Text rendering (simple)
void ssd1306_SetFont(const FontDef *font) { current_font = font; }

void SSD1306_WriteChar(char c) {
    if (c < 32 || c > 126) c = '?';
    uint16_t first = 32;
    uint16_t index = (c - first) * current_font->bytes_per_char + 0;
    const uint8_t *chdata = &current_font->data[index];
    uint8_t w = current_font->width;
    uint8_t h = current_font->height;
    uint8_t bytesPerCol = (h + 7) / 8;
    uint8_t col = 0, row = 0;
    for (col = 0; col < w; col++) {
        for ( row = 0; row < h; row++) {
            uint8_t byte = chdata[col * bytesPerCol + (row / 8)];
            bool pix = (byte >> (row & 7)) & 1;
            SSD1306_DrawPixel(cursor_x + col, cursor_y + row, pix);
        }
    }
    cursor_x += w + 1;
    if (cursor_x + w >= SSD1306_WIDTH) { cursor_x = 0; cursor_y += h + 1; }
}

// --- Buffer / flush helpers ---
void SSD1306_DisplayOnBuffer(void) {
    // alias - nothing extra needed
}

void SSD1306_DisplayFlush(void) {
    SSD1306_Display();
}

void float_to_string(float num, char *buffer, int precision)
{
    // Handle negative numbers
    if (num < 0) {
        *buffer++ = '-';
        num = -num;
    }

    // Apply rounding:
    // Example: for precision=3 → add 0.0005 before truncation
    float rounding = 0.5f;
    int i = 0;
    for (i = 0; i < precision; i++)
        rounding /= 10.0f;

    num += rounding;

    // Extract integer part
    int int_part = (int)num;

    // Extract fractional part
    float frac = num - (float)int_part;

    // Convert integer part
    char temp[16];
    i = 0;
    if (int_part == 0) {
        temp[i++] = '0';
    } else {
        while (int_part > 0) {
            temp[i++] = (int_part % 10) + '0';
            int_part /= 10;
        }
    }

    // Reverse digits
    while (i--)
        *buffer++ = temp[i];

    // If no decimals requested
    if (precision == 0) {
        *buffer = '\0';
        return;
    }

    // Decimal point
    *buffer++ = '.';

    // Fractional digits
    int p=0;
    for (p = 0; p < precision; p++) {
        frac *= 10.0f;
        int digit = (int)frac;
        *buffer++ = digit + '0';
        frac -= digit;
    }

    // End string
    *buffer = '\0';
}
