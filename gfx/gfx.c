#include "pico/stdlib.h"
#include "malloc.h"
#include "stdarg.h"
#include <string.h>
#include "gfx.h"
#include "font.h"
#include "gfxfont.h"

#ifndef swap
#define swap(a, b)     \
	{                  \
		int16_t t = a; \
		a = b;         \
		b = t;         \
	}
#endif

#define GFX_BLACK 0x0000
#define GFX_WHITE 0xFFFF

uint16_t *gfxFramebuffer = NULL;

extern uint16_t _width;	 ///< Display width as modified by current rotation
extern uint16_t _height; ///< Display height as modified by current rotation

static int16_t cursor_y = 0;
int16_t cursor_x = 0;
uint8_t textsize_x = 1;
uint8_t textsize_y = 1;
uint16_t textcolor = GFX_WHITE;
uint16_t textbgcolor = GFX_BLACK;
uint16_t clearColour = GFX_BLACK;
uint8_t wrap = 1;

GFXfont *gfxFont = NULL;

void GFX_setClearColor(uint16_t color)
{
	clearColour = color;
}

void GFX_clearScreen()
{
	GFX_fillRect(0, 0, _width, _height, clearColour);
}

void GFX_fillScreen(uint16_t color)
{
	GFX_fillRect(0, 0, _width, _height, color);
}

void GFX_drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if (gfxFramebuffer != NULL)
	{
		if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
			return;
		gfxFramebuffer[x + y * _width] = color ; //(color >> 8) | (color << 8);
	}
	else
		LCD_WritePixel(x, y, color);
}

void GFX_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{

	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep)
	{
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1)
	{
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if (steep)
		{
			GFX_drawPixel(y0, x0, color);
		}
		else
		{
			GFX_drawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

void GFX_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	GFX_drawLine(x, y, x, y + h - 1, color);
}

void GFX_drawFastHLine(int16_t x, int16_t y, int16_t l, uint16_t color)
{
	GFX_drawLine(x, y, x + l - 1, y, color);
}

void GFX_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	for (int16_t i = x; i < x + w; i++)
	{
		GFX_drawFastVLine(i, y, h, color);
	}
}

void GFX_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	GFX_drawFastHLine(x, y, w, color);
	GFX_drawFastHLine(x, y + h - 1, w, color);
	GFX_drawFastVLine(x, y, h, color);
	GFX_drawFastVLine(x + w - 1, y, h, color);
}

void GFX_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y)
{
	if (!gfxFont)
	{
		if ((x >= _width) ||			  // Clip right
			(y >= _height) ||			  // Clip bottom
			((x + 6 * size_x - 1) < 0) || // Clip left
			((y + 8 * size_y - 1) < 0))	  // Clip top
			return;

		if (c >= 176)
			c++; // Handle 'classic' charset behavior

		// GFX_Select();
		for (int8_t i = 0; i < 5; i++)
		{ // Char bitmap = 5 columns
			uint8_t line = font[c * 5 + i];
			for (int8_t j = 0; j < 8; j++, line >>= 1)
			{
				if (line & 1)
				{
					if (size_x == 1 && size_y == 1)
						GFX_drawPixel(x + i, y + j, color);
					else
						GFX_fillRect(x + i * size_x, y + j * size_y, size_x,
									 size_y, color);
				}
				else if (bg != color)
				{
					if (size_x == 1 && size_y == 1)
						GFX_drawPixel(x + i, y + j, bg);
					else
						GFX_fillRect(x + i * size_x, y + j * size_y, size_x,
									 size_y, bg);
				}
			}
		}
		if (bg != color)
		{ // If opaque, draw vertical line for last column
			if (size_x == 1 && size_y == 1)
				GFX_drawFastVLine(x + 5, y, 8, bg);
			else
				GFX_fillRect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
		}
		// GFX_DeSelect();
	}
	else
	{
		c -= (uint8_t)gfxFont->first;
		GFXglyph *glyph = (gfxFont->glyph) + c;
		uint8_t *bitmap = gfxFont->bitmap;

		uint16_t bo = glyph->bitmapOffset;
		uint8_t w = glyph->width, h = glyph->height;
		int8_t xo = glyph->xOffset, yo = glyph->yOffset;
		uint8_t xx, yy, bits = 0, bit = 0;
		int16_t xo16 = 0, yo16 = 0;

		if (size_x > 1 || size_y > 1)
		{
			xo16 = xo;
			yo16 = yo;
		}

		// GFX_Select();
		for (yy = 0; yy < h; yy++)
		{
			for (xx = 0; xx < w; xx++)
			{
				if (!(bit++ & 7))
				{
					bits = bitmap[bo++];
				}
				if (bits & 0x80)
				{
					if (size_x == 1 && size_y == 1)
					{
						GFX_drawPixel(x + xo + xx, y + yo + yy, color);
					}
					else
					{
						GFX_fillRect(x + (xo16 + xx) * size_x,
									 y + (yo16 + yy) * size_y, size_x, size_y,
									 color);
					}
				}
				bits <<= 1;
			}
		}
		// GFX_DeSelect();
	}
}

void GFX_write(uint8_t c)
{
	if (!gfxFont)
	{
		if (c == '\n')
		{								// Newline?
			cursor_x = 0;				// Reset x to zero,
			cursor_y += textsize_y * 8; // advance y one line
		}
		else if (c != '\r')
		{ // Ignore carriage returns
			if (wrap && ((cursor_x + textsize_x * 6) > _width))
			{								// Off right?
				cursor_x = 0;				// Reset x to zero,
				cursor_y += textsize_y * 8; // advance y one line
			}
			GFX_drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor,
						 textsize_x, textsize_y);
			cursor_x += textsize_x * 6; // Advance x one char
		}
	}
	else
	{
		if (c == '\n')
		{
			cursor_x = 0;
			cursor_y += (int16_t)textsize_y * (uint8_t)(gfxFont->yAdvance);
		}
		else if (c != '\r')
		{
			uint8_t first = gfxFont->first;
			if ((c >= first) && (c <= (uint8_t)gfxFont->last))
			{
				GFXglyph *glyph = (gfxFont->glyph) + (c - first);
				uint8_t w = glyph->width, h = glyph->height;
				if ((w > 0) && (h > 0))
				{										 // Is there an associated bitmap?
					int16_t xo = (int8_t)glyph->xOffset; // sic
					if (wrap && ((cursor_x + textsize_x * (xo + w)) > _width))
					{
						cursor_x = 0;
						cursor_y += (int16_t)textsize_y * (uint8_t)gfxFont->yAdvance;
					}
					GFX_drawChar(cursor_x, cursor_y, c, textcolor,
								 textbgcolor, textsize_x, textsize_y);
				}
				cursor_x += (uint8_t)glyph->xAdvance * (int16_t)textsize_x;
			}
		}
	}
}

void GFX_setCursor(int16_t x, int16_t y)
{
	cursor_x = x;
	cursor_y = y;
}

void GFX_setTextColor(uint16_t color)
{
	textcolor = color;
}

void GFX_setTextBack(uint16_t color)
{
	textbgcolor = color;
}

void GFX_setFont(const GFXfont *f)
{
	if (f)
	{ // Font struct pointer passed in?
		if (!gfxFont)
		{ // And no current font struct?
			// Switching from classic to new font behavior.
			// Move cursor pos down 6 pixels so it's on baseline.
			cursor_y += 6;
		}
	}
	else if (gfxFont)
	{ // NULL passed.  Current font struct defined?
		// Switching from new to classic font behavior.
		// Move cursor pos up 6 pixels so it's at top-left of char.
		cursor_y -= 6;
	}
	gfxFont = (GFXfont *)f;
}

void fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
					  uint8_t corners, int16_t delta,
					  uint16_t color)
{

	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;
	int16_t px = x;
	int16_t py = y;

	delta++; // Avoid some +1's in the loop

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		// These checks avoid double-drawing certain lines, important
		// for the SSD1306 library which has an INVERT drawing mode.
		if (x < (y + 1))
		{
			if (corners & 1)
				GFX_drawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
			if (corners & 2)
				GFX_drawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
		}
		if (y != py)
		{
			if (corners & 1)
				GFX_drawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
			if (corners & 2)
				GFX_drawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
			py = y;
		}
		px = x;
	}
}

void GFX_fillCircle(int16_t x0, int16_t y0, int16_t r,
					uint16_t color)
{

	GFX_drawFastVLine(x0, y0 - r, 2 * r + 1, color);
	fillCircleHelper(x0, y0, r, 3, 0, color);
}

void GFX_drawCircle(int16_t x0, int16_t y0, int16_t r,
					uint16_t color)
{

	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	GFX_drawPixel(x0, y0 + r, color);
	GFX_drawPixel(x0, y0 - r, color);
	GFX_drawPixel(x0 + r, y0, color);
	GFX_drawPixel(x0 - r, y0, color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		GFX_drawPixel(x0 + x, y0 + y, color);
		GFX_drawPixel(x0 - x, y0 + y, color);
		GFX_drawPixel(x0 + x, y0 - y, color);
		GFX_drawPixel(x0 - x, y0 - y, color);
		GFX_drawPixel(x0 + y, y0 + x, color);
		GFX_drawPixel(x0 - y, y0 + x, color);
		GFX_drawPixel(x0 + y, y0 - x, color);
		GFX_drawPixel(x0 - y, y0 - x, color);
	}
}

char printBuf[100];
void printString(char s[])
{
	uint8_t n = strlen(s);
	for (int i = 0; i < n; i++)
		GFX_write(s[i]);
}

void GFX_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vsprintf(printBuf, format, args);
	printString(printBuf);
	va_end(args);
}

void GFX_createFramebuf()
{
	gfxFramebuffer = malloc(_width * _height * sizeof(uint16_t));
}
void GFX_destroyFramebuf()
{
	free(gfxFramebuffer);
	gfxFramebuffer = NULL;
}

void GFX_flush()
{
	if (gfxFramebuffer != NULL)
		LCD_WriteBitmap(0, 0, _width, _height, gfxFramebuffer);
}

void GFX_WriteBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *bitmap){
    LCD_WriteBitmap(x, y, w, h, bitmap);
}


