/*
 * sharp_graphics.h
 *
 * Graphics operations for Sharp Memory LCD
 */

#ifndef INC_SHARP_GRAPHICS_H_
#define INC_SHARP_GRAPHICS_H_

#include "fonts.h"
#include "sharp.h"
#include "orcos.h"      // For LCD_HEIGHT, LCD_WIDTH

extern uint8_t g_framebuffer[LCD_HEIGHT][LCD_WIDTH / 8];

/* Drawing functions */
void lcd_putsAt(const char *str, uint8_t font_id, uint16_t dx, uint16_t dy, uint8_t color);
void lcd_draw_img(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint8_t color);
void bitblt24(uint32_t x, uint32_t dx, uint32_t y, uint32_t val, int blt_op, int fill);
void lcd_fill_rect(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, int val);
void lcd_invert_framebuffer(void);
void lcd_draw_test_pattern(uint8_t square_size);
void lcd_fill(uint8_t color);
void lcd_clear_buffer(void);
FontDef_t *font_lookup(uint8_t font_id);

/* Helper functions */
uint8_t reverse_bits(uint8_t b);

#endif /* INC_SHARP_GRAPHICS_H_ */
