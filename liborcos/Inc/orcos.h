/**
 * @file orcos.h 
 * @brief OpenRPNCalc Operating System (ORCOS) library header
 *
 * This is a subset of functions from dmcp.h, distributed under BSD-3 license.
 * Provides hardware abstraction and core functionality for OpenRPNCalc.
 * Original source: https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h
 */

#ifndef __ORCOS_H
#define __ORCOS_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Get battery voltage in millivolts
 * @return Battery voltage in mV
 */
int get_vbat(void);

// LCD dimensions and buffer sizes
#define LCD_WIDTH 400    ///< Display width in pixels
#define LCD_HEIGHT 240   ///< Display height in pixels 
#define LCD_LINE_SIZE 50 ///< Bytes per line in framebuffer
#define LCD_LINE_BUF_SIZE (LCD_LINE_SIZE + 4) ///< Buffer size for line transfers

/* Color Constants */
#define LCD_SET_VALUE 0 // '1' bits in the given buffer will activate display pixels (black)
#define LCD_EMPTY_VALUE ~(0) // '1' bits in the given buffer will clear display pixels (white)

/* Font identifiers */
#define FONT_6x8 0
#define FONT_7x12b 1
#define FONT_12x20 2
#define FONT_24x40 3
#define FONT_16x26 4

/**
 * @brief Power on the LCD display
 * Enables power supply and initializes display controller
 */
void LCD_power_on(void);

/**
 * @brief Power off the LCD display
 * @param clear If non-zero, clears display before power off
 */
void LCD_power_off(int clear);

/**
 * @brief Check if LCD is powered on
 * @return true if LCD is powered on, false otherwise
 */
bool LCD_is_on(void);

/// Blit operation types
#define BLT_OR    0  ///< OR operation (set pixels)
#define BLT_ANDN  1  ///< AND-NOT operation (clear pixels)
#define BLT_XOR   2  ///< XOR operation (toggle pixels)

/// Fill modes for blit operations  
#define BLT_NONE  0  ///< No fill - use source value
#define BLT_SET   1  ///< Fill with constant value

// Note: Most drawing functions are well-documented in sharp_graphics.c
// Only adding brief descriptions here to avoid duplication

/// Draw a 24-bit wide bitblt operation
void bitblt24(uint32_t x, uint32_t dx, uint32_t y, uint32_t val, int blt_op, int fill);

/// Write a line buffer to LCD
void LCD_write_line(uint8_t *buf);

/// Draw an image to the framebuffer
void lcd_draw_img(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint8_t color);

/// Refresh the LCD with current framebuffer contents
void lcd_refresh(void);

/// Fill screen with test pattern of given square size
void lcd_draw_test_pattern(uint8_t square_size);

/// Fill entire framebuffer with pattern
void lcd_fill(uint8_t fill_pattern);

/// Clear framebuffer (fill with LCD_EMPTY_VALUE)
void lcd_clear_buffer(void);

/// Invert all pixels in framebuffer
void lcd_invert_framebuffer(void);

/// Draw text string at specified position
void lcd_putsAt(const char *str, uint8_t font_id, uint16_t dx, uint16_t dy, uint8_t color);

/// Draw filled rectangle
void lcd_fill_rect(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, int val);

/// Draw calculator screen based on mode
int lcd_for_calc(int what_screen);

/** \addtogroup SCREENS
 * @{
 */
#define 	DISP_CALC   0
#define 	DISP_SYS_MENU   2
#define 	DISP_BOOTLOADER   4
#define 	DISP_UNIMPLEMENTED   5
#define 	DISP_USB_WRITE   6
#define 	DISP_MSC_CONNECT_USB   7
#define 	DISP_ABOUT   8
#define 	DISP_FAT_FORMAT   9
#define 	DISP_FAULT   11
#define 	DISP_QSPI_BAD_CRC   12
#define 	DISP_QSPI_CHECK   13
#define 	DISP_MARK_REGION   15
#define 	DISP_DISK_TEST   16
#define 	DISP_DSKTST_CONNECT_USB   17
#define 	DISP_QSPI_CONNECT_USB   18
#define 	DISP_OFF_IMAGE_ERR   19
#define 	DISP_HELP   21
#define 	DISP_BOOTLDR_CON_USB   22
#define 	DISP_PROD_DIAG   23
#define 	DISP_POWER_CHECK   24
#define 	DISP_FLASH_CONNECT_USB   26
/** @} */

/**
 * @brief Put calculator into low-power sleep mode
 * @param off If non-zero, only wake on ON key. If zero, wake on any key.
 */
void sys_sleep(int off);

/**
 * @brief Perform system reset
 * Immediately resets the microcontroller
 */
void sys_reset(void);

/**
 * @brief Initialize ORCOS library
 * Sets up hardware peripherals, clocks, and display
 */
void orcos_init();

#if DEBUG
#define DEBUG_PRINT(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

/**
 * @brief Scan keyboard matrix
 * @return Keycode of pressed key, 0 if none, 0xFFFF if multiple keys
 */
uint16_t scan_keyboard(void);

/**
 * @brief Pop keycode from input queue
 * @return Next keycode in queue, 0 if empty
 */
uint16_t key_pop(void);

/**
 * @brief Push keycode to input queue
 * @param keycode Keycode to enqueue
 */
void key_push(uint16_t keycode);

/**
 * @brief Wait until a key is pressed
 * Puts system to sleep until key press detected
 * Use key_pop() to retrieve the keycode
 */
void wait_for_key_press();

/** \addtogroup KEYCODES 
 * @{
 */
#define KEY_0 1
#define KEY_SIGN 2
#define KEY_ENTER 29
#define KEY_F 49
#define KEY_ON 54
/** @} */

#define MULTIPLE_KEYS_PRESSED(x)  ((x) > 0xff)
#define FIRST_KEYCODE(x)  ((x) & 0xff)
#define SECOND_KEYCODE(x)  ((x) >> 8)
#define TOO_MANY_KEYS_PRESSED(x)  ((x) == 0xffff)


// Date/Time functions 
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
