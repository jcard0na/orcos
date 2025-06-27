/*
 * sharp_graphics.c
 *
 * Graphics operations for Sharp Memory LCD
 */

#include "sharp_graphics.h"
#include "fonts.h"
#include "sharp.h"
#include "orcos.h"
#include <string.h>
#include <stdbool.h>

extern uint8_t g_framebuffer[LCD_HEIGHT][LCD_WIDTH / 8];

static void lcd_draw_img_aligned(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint8_t color, bool msb);
static void lcd_draw_img_unaligned(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint8_t color, bool msb);

FontDef_t *font_lookup(uint8_t font_id)
{
    switch (font_id)
    {
    case FONT_6x8:
        return &font_6x8;
    case FONT_7x12b:
        return &font_7x12b;
    case FONT_12x20:
        return &font_12x20;
    case FONT_16x26:
        return &font_16x26;
    case FONT_24x40:
        return &font_24x40;
    default:
        return NULL; // Invalid font ID
    }
}

void lcd_putsAt(const char *str, uint8_t font_id, uint16_t dx, uint16_t dy, uint8_t color)
{
    FontDef_t *font = font_lookup(font_id);
    if (font == NULL)
        return;

    uint16_t width = font->FontWidth;
    uint16_t height = font->FontHeight;
    uint16_t bytes_per_char = (width + 7) / 8;
    const char (*font_data)[bytes_per_char * height] = font->data;

    if (dy >= LCD_HEIGHT)
        return;

    uint16_t xpos = ((dx + 7) / 8) << 3;
    int char_idx = 0;

    // Temporary buffer for one character
    uint8_t char_buffer[bytes_per_char * height];

    while (xpos < LCD_WIDTH && str[char_idx] != '\0')
    {
        uint8_t current_char = str[char_idx];
        const char *char_data = font_data[current_char];

        // Copy character data directly (already in MSB format)
        memcpy(char_buffer, char_data, bytes_per_char * height);

        // Draw character using MSB-optimized function
        lcd_draw_img_unaligned(char_buffer, width, height, xpos, dy, color, true);

        xpos += width;
        char_idx++;
    }
}

void lcd_draw_img(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint8_t color)
{
    if (img == NULL || x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;

    if ((x % 8) == 0 && (w % 8) == 0)
    {
        lcd_draw_img_aligned(img, w, h, x, y, color, false);
    }
    else
    {
        lcd_draw_img_unaligned(img, w, h, x, y, color, false);
    }
}

static void lcd_draw_img_aligned(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint8_t color, bool msb)
{
    uint32_t img_stride = w / 8;
    uint32_t dest_x = x / 8;

    for (uint32_t dy = 0; dy < h; dy++)
    {
        uint32_t dest_y = y + dy;
        if (dest_y >= LCD_HEIGHT)
            break;

        for (uint32_t sx = 0; sx < img_stride; sx++)
        {
            uint8_t src_byte = msb ? img[dy * img_stride + sx] : reverse_bits(img[dy * img_stride + sx]);
            if (src_byte)
            {
                uint32_t dx = dest_x + sx;
                if (dx < (LCD_WIDTH / 8))
                {
                    if (color == LCD_SET_VALUE)
                    {
                        g_framebuffer[dest_y][dx] &= ~src_byte; // Clear bits for white
                    }
                    else
                    {
                        g_framebuffer[dest_y][dx] |= src_byte; // Set bits for black
                    }
                }
            }
        }
    }
}

static void lcd_draw_img_unaligned(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint8_t color, bool msb)
{
    uint32_t img_stride = (w + 7) / 8;
    uint8_t x_align = x % 8;

    for (uint32_t dy = 0; dy < h; dy++)
    {
        uint32_t dest_y = y + dy;
        if (dest_y >= LCD_HEIGHT)
            continue;

        for (uint32_t sx = 0; sx < img_stride; sx++)
        {
            uint8_t img_byte = msb ? img[dy * img_stride + sx] : reverse_bits(img[dy * img_stride + sx]);
            if (img_byte == 0)
                continue;

            uint32_t dest_byte = (x / 8) + sx;
            uint8_t shift = x_align;

            if (dest_byte < (LCD_WIDTH / 8))
            {
                uint8_t mask = img_byte << shift;
                if (color == LCD_SET_VALUE)
                {
                    g_framebuffer[dest_y][dest_byte] &= ~mask; // Clear bits
                }
                else
                {
                    g_framebuffer[dest_y][dest_byte] |= mask; // Set bits
                }
            }

            if (shift != 0 && dest_byte + 1 < (LCD_WIDTH / 8))
            {
                uint8_t mask = img_byte >> (8 - shift);
                if (color == LCD_SET_VALUE)
                {
                    g_framebuffer[dest_y][dest_byte + 1] &= ~mask;
                }
                else
                {
                    g_framebuffer[dest_y][dest_byte + 1] |= mask;
                }
            }
        }
    }
}

void bitblt24(uint32_t x, uint32_t dx, uint32_t y, uint32_t val, int blt_op, int fill)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT || dx == 0 || dx > 24)
        return;

    // Clamp dx to remaining width
    if (x + dx > LCD_WIDTH)
        dx = LCD_WIDTH - x;

    // Apply fill mode to source value
    if (fill == BLT_SET)
    {
        if (blt_op == BLT_OR)
        {
            val = 0;
        }
        else if (blt_op == BLT_ANDN)
        {
            val = ~0;
        }
    }

    // Process each bit
    for (uint32_t i = 0; i < dx; i++)
    {
        uint32_t bit_pos = x + i;
        uint8_t bit_val = (val >> (dx - 1 - i)) & 1;
        uint8_t *byte_ptr = &g_framebuffer[y][bit_pos / 8];
        uint8_t bit_mask = 1 << bit_pos % 8;

        switch (blt_op)
        {
        case BLT_OR:
            if (bit_val)
            {
                *byte_ptr |= bit_mask; // Set bit (black pixel)
            }
            else if (fill == BLT_SET)
            {
                *byte_ptr &= ~bit_mask; // Clear bit (white pixel)
            }
            break;
        case BLT_ANDN:
            if (!bit_val)
            {
                *byte_ptr &= ~bit_mask;
            }
            else if (fill == BLT_SET)
            {
                *byte_ptr |= bit_mask;
            }
            break;
        case BLT_XOR:
            if (bit_val)
            {
                *byte_ptr ^= bit_mask;
            }
            break;
        }
    }
}

uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void lcd_fill_rect(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, int val)
{
    // Clamp to screen bounds
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;
    if (x + dx > LCD_WIDTH)
        dx = LCD_WIDTH - x;
    if (y + dy > LCD_HEIGHT)
        dy = LCD_HEIGHT - y;

    for (uint32_t curr_y = y; curr_y < y + dy; curr_y++)
    {
        for (uint32_t curr_x = x; curr_x < x + dx; curr_x++)
        {
            uint8_t *byte_ptr = &g_framebuffer[curr_y][curr_x / 8];
            uint8_t bit_mask = 1 << (curr_x % 8);

            if (val == LCD_SET_VALUE)
            {
                *byte_ptr &= ~bit_mask; // Clear bit (black pixel)
            }
            else
            {
                *byte_ptr |= bit_mask; // Set bit (white pixel)
            }
        }
    }
}

void lcd_invert_framebuffer(void)
{
    for (int y = 0; y < LCD_HEIGHT; y++)
    {
        for (int x_byte = 0; x_byte < (LCD_WIDTH / 8); x_byte++)
        {
            g_framebuffer[y][x_byte] = ~g_framebuffer[y][x_byte];
        }
    }
}

void lcd_draw_test_pattern(uint8_t square_size)
{
    // Ensure square_size is at least 1 and not too large
    if (square_size < 1)
        square_size = 1;
    if (square_size > 32)
        square_size = 32;

    for (int y = 0; y < LCD_HEIGHT; y++)
    {
        for (int x = 0; x < LCD_WIDTH; x++)
        {
            // Calculate checkerboard pattern
            uint8_t x_block = x / square_size;
            uint8_t y_block = y / square_size;
            uint8_t pattern = (x_block + y_block) % 2;

            // Set pixel in framebuffer
            if (pattern)
            {
                g_framebuffer[y][x / 8] |= (1 << (7 - (x % 8))); // Set pixel white
            }
            else
            {
                g_framebuffer[y][x / 8] &= ~(1 << (7 - (x % 8))); // Set pixel black
            }
        }
    }
}

void lcd_fill(uint8_t color)
{
    if (color == LCD_SET_VALUE)
    {
        memset(g_framebuffer, 0x00, sizeof(g_framebuffer));
    }
    else
    {
        memset(g_framebuffer, 0xff, sizeof(g_framebuffer));
    }
}

void lcd_clear_buffer(void)
{
    lcd_fill(LCD_EMPTY_VALUE);
}
