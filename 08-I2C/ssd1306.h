#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>

// Configure display size here:
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 32   // set to 32 for 128x32 modules

// I2C default address for many OLED modules:
#define SSD1306_I2C_ADDR 0x3C

// User provides which I2C base (e.g. I2C0_BASE) and a small delay function if needed
// Init call requires the Tiva I2C base address and the I2C address of the display.
void SSD1306_Init(uint32_t i2c_base, uint8_t i2c_addr);

// Low-level I2C control (exposed in case you need)
void SSD1306_Reset(void);
void SSD1306_Display(void);
void SSD1306_Clear(void);
void SSD1306_Invert(bool invert);
void SSD1306_SetContrast(uint8_t contrast);

// Drawing primitives
void SSD1306_DrawPixel(int16_t x, int16_t y, bool color);
void SSD1306_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color);
void SSD1306_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color);
void SSD1306_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color);
void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, bool color);

// Bitmap (monochrome): width in pixels, height in pixels, bitmap array in bytes (column-major or row-major as produced by typical tools)
void SSD1306_DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, bool color);

// Text
#include "ssd1306_fonts.h"
void ssd1306_SetFont(const FontDef *font);
void SSD1306_SetCursor(int16_t x, int16_t y);
void SSD1306_WriteChar(char c);
void SSD1306_WriteString(const char *s);
void SSD1306_WriteInt(int32_t val);
void SSD1306_WriteFloat(float val, uint8_t decimals);;

// Advanced: set to true to buffer writes and call Display() to flush
void SSD1306_DisplayOnBuffer(void);
void SSD1306_DisplayFlush(void);

#endif // SSD1306_H
