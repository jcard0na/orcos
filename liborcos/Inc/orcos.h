// Header file for the ORCOS library
// This is a subset of functions from dmcp.h
// https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h
//
// That header file is distributed under BSD-3

#ifndef __ORCOS_H
#define __ORCOS_H

#include <stdbool.h>

int get_vbat(void);

#define LCD_WIDTH 400
#define LCD_HEIGHT 240
#define LCD_LINE_SIZE 50
#define LCD_LINE_BUF_SIZE (LCD_LINE_SIZE + 4)

/* Color Constants */
#define BLACK 0 // '1' bits will set display pixels (black)
#define WHITE 1 // '1' bits will clear display pixels (white)

/* Font identifiers */
#define FONT_6x8 0
#define FONT_7x12b 1
#define FONT_12x20 2
#define FONT_24x40 3
#define FONT_16x26 4

void LCD_power_on(void);
void LCD_power_off(int clear);
bool LCD_is_on(void);

// Sends one line data to LCD
void LCD_write_line(uint8_t *buf);
void lcd_draw_img(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint8_t color);
void lcd_draw_img_msb(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint8_t color);
void lcd_refresh(void);
void lcd_draw_test_pattern(uint8_t square_size);
void lcd_fill(uint8_t fill_pattern);
void lcd_clear_buffer(void);
void lcd_invert_framebuffer(void);
void lcd_putsAt(const char *str, uint8_t font_id, uint16_t dx, uint16_t dy, uint8_t color);

// Put calculator to sleep
// off: if non-zero, only wake on ON key
// off: if zero, wake on any key
void sys_sleep(int off);

void WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) __attribute__((used, noinline));

// Main library init function
void orcos_init();

#if DEBUG
#define DEBUG_PRINT(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

void switch_input();
uint16_t scan_keyboard(void);

// RTC
typedef struct
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
} dt_t;

typedef struct
{
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t csec;
  uint8_t dow;
} tm_t;

void rtc_read(tm_t *tm, dt_t *dt);

// Deprecated

void calc_init(void);
int calc_on_key(int);

#endif /* __ORCOS_H */