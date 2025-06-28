/*
 * sharp.c
 *
 * Functions to work with Sharp Memory LCD of type LS027B7DH01.
 *
 * The display in OpenRPNCalc is installed upside down for convenience of PCB routing,
 * so this has to be taken into account in the firmware.
 *
 *  Created on: Mar 9, 2021
 *      Author: apolu
 */

#include "stm32u3xx_hal.h"
#include "fonts.h"
#include "sharp.h"
#include "sharp_graphics.h"
#include "sharp_lowlevel.h"
#include "stm32u3xx_hal_rtc_ex.h"
#include "openrpncalc.h"
#include "orcos.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#if DEBUG
#include "SEGGER_RTT.h"
#endif

extern RTC_HandleTypeDef hrtc;
TIM_HandleTypeDef htim1;
SPI_HandleTypeDef hspi2;
LPTIM_HandleTypeDef hlptim1;


// 1bpp packed buffer (400x240 / 8 = 12,000 bytes)
uint8_t g_framebuffer[LCD_HEIGHT][LCD_WIDTH / 8];

// Power management variables
static int timeout_counter = 0;
static bool lcd_is_on = false;
static int current_test_screen = 0;
#define OFF_TIMEOUT (5 * 60) // 5 min timeout before switching off


/**
 * @brief Draws a string directly to the LCD framebuffer
 * @param str: String to draw
 * @param font: Pointer to font definition
 * @param dx: X position (0-399)
 * @param dy: Y position (0-239)
 * @param color: LCD_SET_VALUE (1=set) or LCD_EMPTY_VALUE (1=clear)
 *
 * Note: dx will be rounded to the closest byte-aligned address
 */

void WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    // DEBUG_PRINT("-- LPTIM1: EXTIN toggle ---\n");
    RTC_TimeTypeDef Time;
    RTC_DateTypeDef Date;
    /* Get the RTC calendar time */
    HAL_RTC_GetTime(hrtc, &Time, RTC_FORMAT_BIN);
    // Do not skip reading the date: This triggers an update to the shadow registers!
    HAL_RTC_GetDate(hrtc, &Date, RTC_FORMAT_BIN);
#if DEBUG
    SEGGER_RTT_printf(0, "wake up timer event %02d:%02d! (%d)\n", Time.Minutes, Time.Seconds, timeout_counter);
#endif

    // If we are showng the RTC test screen, update the time
    if (current_test_screen == 5)
    {
        LCD_test_screen(current_test_screen);
    }

    timeout_counter++;
    if (timeout_counter > OFF_TIMEOUT)
    {
        LCD_power_off(1);
    }
}

void lcd_keep_alive()
{
    timeout_counter = 0;
}

void LCD_power_on()
{
    DEBUG_PRINT("\n--- LDC_power_on() ---\n");
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET); // 5V booster enable
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET); // DISP signal to "ON"
    /* Configure wakeup interrupt */
    /* RTC Wakeup Interrupt Generation:
      (2047 + 1) × (16 / 32768) = 1.000 seconds
    */
    if (HAL_RTC_RegisterCallback(&hrtc, HAL_RTC_WAKEUPTIMER_EVENT_CB_ID, WakeUpTimerEventCallback) != HAL_OK)
    {
        LCD_Error_Handler();
    }
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2047, RTC_WAKEUPCLOCK_RTCCLK_DIV16, 0);
    lcd_is_on = true;
}

void LCD_power_off(int clear)
{
    DEBUG_PRINT("\n--- LDC_power_off() ---\n");
    // XXX: this prevents waking up form STOP2
    // HAL_TIM_Base_Stop_IT(&htim1); // Stop the timer
    delay_us(30);
    if (clear)
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // DISP signal to "OFF"
    delay_us(30);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);  // EXTCOMIN signal of "OFF"
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); // 5V booster disable
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    lcd_is_on = false;
}

bool LCD_is_on()
{
    return lcd_is_on;
}




/**
 * Inverts all pixels in the global framebuffer (black becomes white and vice versa)
 */

// Sends line data to LCD.
//
// Parameters
//     buf	Buffer with line data
//
// Buffer should be of LCD_LINE_BUF_SIZE.
//
// With following values:
//
//     [0] - reserved space for LCD write command
//     [1] - LCD line number
//     [2..51] - 50 bytes of LCD line data
//     [52..53] - padding required by LCD hw
//
// Only line number and line data have to by filled by user.


int lcd_for_calc(int what_screen)
{
    switch (what_screen)
    {
    case DISP_SYS_MENU:
        lcd_clear_buffer();

        // Get and display RTC time
        tm_t time;
        dt_t date;
        rtc_read(&time, &date);

        char time_str[20];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                 time.hour, time.min, time.sec);
        lcd_putsAt(time_str, FONT_16x26, 120, 100, LCD_SET_VALUE);

        char date_str[20];
        snprintf(date_str, sizeof(date_str), "%02d/%02d/20%02d",
                 date.day, date.month, date.year);
        lcd_putsAt(date_str, FONT_16x26, 120, 130, LCD_SET_VALUE);

        char voltage_str[20];
        uint16_t vbat = (uint16_t)get_vbat();
        snprintf(voltage_str, sizeof(voltage_str), "bat: %01d.%03dV",
                 vbat / 1000, vbat % 1000);
        lcd_putsAt(voltage_str, FONT_16x26, 0, 0, LCD_SET_VALUE);

        break;
    default:
        lcd_draw_test_pattern(8);
    }
    return LCD_HEIGHT;
}

void LCD_test_screen(uint16_t count)
{
#define NUM_OF_TEST_SCREENS 9
    count = (count % NUM_OF_TEST_SCREENS);
    current_test_screen = count;
    DEBUG_PRINT("Test screen %d\n", count);
    if (count == 0)
    {
        lcd_draw_test_pattern(8);
    }
    if (count == 1)
    {
        lcd_draw_test_pattern(16);
    }
    if (count == 2)
    {
        lcd_draw_test_pattern(32);
    }
    if (count == 3)
    {
        lcd_draw_test_pattern(32);
        for (int i = 0; i < 7; i++)
        {
            for (int j = 0; j < 6; j++)
            {
                lcd_draw_img(rook_img, 32, 32, i * 64, j * 64, LCD_EMPTY_VALUE);
                lcd_draw_img(rook_img, 32, 32, i * 64, j * 64 + 32, LCD_SET_VALUE);
                lcd_draw_img(rook_img, 32, 32, i * 64 + 32, j * 64 + 32, LCD_EMPTY_VALUE);
                lcd_draw_img(rook_img, 32, 32, i * 64 + 32, j * 64, LCD_SET_VALUE);
            }
        }
    }
    if (count == 4)
    {
        lcd_clear_buffer();
        lcd_draw_img(pixel_data_bin, 400, 240, 0, 0, LCD_SET_VALUE);
        lcd_draw_img(test_img, 32, 32, 0, 0, LCD_SET_VALUE);
        lcd_draw_img(test_img, 32, 32, 50, 50, LCD_SET_VALUE);
        lcd_draw_img(test_img, 32, 32, 90, 90, LCD_EMPTY_VALUE);

        lcd_draw_img((uint8_t[]){0xff, 0xff}, 2, 2, 100, 100, LCD_SET_VALUE);
        lcd_draw_img((uint8_t[]){0xff, 0xff}, 2, 2, 106, 100, LCD_SET_VALUE);
        lcd_draw_img((uint8_t[]){0xff, 0xff}, 2, 2, 100, 106, LCD_SET_VALUE);
        lcd_draw_img((uint8_t[]){0xff, 0xff}, 2, 2, 106, 106, LCD_SET_VALUE);
    }
    if (count == 5)
    {
        lcd_for_calc(DISP_SYS_MENU);
    }
    if (count == 6)
    {
        lcd_clear_buffer();

        lcd_putsAt("OpenRPNCalc", FONT_24x40, 72, 40, LCD_SET_VALUE);
        lcd_putsAt("Open Hardware", FONT_16x26, 96, 80, LCD_SET_VALUE);
        lcd_putsAt("Hello World!", FONT_12x20, 110, 120, LCD_SET_VALUE);
        lcd_putsAt("Small and Bold!", FONT_7x12b, 130, 140, LCD_SET_VALUE);
        lcd_putsAt("tiny, tiny, behold!", FONT_6x8, 125, 160, LCD_SET_VALUE);
        lcd_putsAt("tiny, tiny, behold!", FONT_6x8, 125, 170, LCD_SET_VALUE);
        lcd_putsAt("tiny, tiny, behold!", FONT_6x8, 125, 180, LCD_SET_VALUE);
        lcd_putsAt("tiny, tiny, behold!", FONT_6x8, 125, 190, LCD_SET_VALUE);
    }
    if (count == 7)
    {
        lcd_clear_buffer();
        // First line - normal black text
        lcd_putsAt("Reverse", FONT_24x40, 120, 80, LCD_SET_VALUE);

        // Second line - highlighted text with background
        FontDef_t *font = font_lookup(FONT_24x40);
        uint16_t text_width = strlen("Polish") * font->FontWidth;
        uint16_t text_height = font->FontHeight;
        uint16_t padding = 10;

        // Draw text first
        lcd_putsAt("Polish", FONT_24x40, 120, 120, LCD_SET_VALUE);

        // XOR the entire text area (background and text)
        for (uint16_t y = 120 - padding / 2; y < 120 + text_height + padding / 2; y++)
        {
            uint16_t remaining_width = text_width + padding;
            uint16_t x_pos = 120 - padding / 2;
            while (remaining_width > 0)
            {
                uint16_t chunk_width = (remaining_width > 24) ? 24 : remaining_width;
                bitblt24(x_pos, chunk_width, y, 0xFFFFFFFF, BLT_XOR, BLT_NONE);
                x_pos += chunk_width;
                remaining_width -= chunk_width;
            }
        }

        // Add rectangle demonstration
        int color = LCD_SET_VALUE;
        for (int i = 0; i < 4; i++)
        {
            lcd_fill_rect(10 + 10 * i, 10 + 5 * i, 70 - 20 * i, 70 - 20 * i, color);
            color = ~color;
        }
    }
    if (count == 8)
    {
        // Draw test image line by line using LCD_write_line
        uint8_t line_buffer[LCD_LINE_BUF_SIZE] = {0};

        // Loop through each line of the test image
        for (int y = 0; y < 240; y++)
        {
            // Set line number (1-based)
            line_buffer[1] = y + 1;

            // Copy one line swapping byte order
            for (int x = 0; x < LCD_LINE_SIZE; x++)
            {
                uint8_t original = pixel_data_bin[y * LCD_LINE_SIZE + x];
                // Reverse bits in each byte
                line_buffer[2 + x] = ((original & 0x01) << 7) |
                                     ((original & 0x02) << 5) |
                                     ((original & 0x04) << 3) |
                                     ((original & 0x08) << 1) |
                                     ((original & 0x10) >> 1) |
                                     ((original & 0x20) >> 3) |
                                     ((original & 0x40) >> 5) |
                                     ((original & 0x80) >> 7);
                line_buffer[2 + x] = ~line_buffer[2 + x];
            }

            // Send the line
            LCD_write_line(line_buffer);
        }
    }
    // Screens that call LCD_write_line(line_buffer) do not need refresh
    if (count != 8)
        lcd_refresh();
}
