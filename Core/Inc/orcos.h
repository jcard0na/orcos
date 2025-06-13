// Header file for the ORCOS library
// This is a subset of functions from dmcp.h
// https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h
//
// That header file is distributed under BSD-3

int get_vbat(void);

#define LCD_WIDTH   400
#define LCD_HEIGHT  240

void LCD_power_on(void);
void LCD_power_off(int clear);
void lcd_draw_img(const uint8_t *img, uint32_t xo, uint32_t yo,
    uint32_t w, uint32_t h, uint32_t x, uint32_t y);
void lcd_refresh(void);
void lcd_draw_test_pattern(void);

// Put calculator to sleep
// off: if non-zero, only wake on ON key
// off: if zero, wake on any key
void sys_sleep(int off);

// Main library init function
void orcos_init();
