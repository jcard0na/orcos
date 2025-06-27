/*
 * sharp_lowlevel.h
 *
 * Low-level hardware operations for Sharp Memory LCD
 */

#ifndef INC_SHARP_LOWLEVEL_H_
#define INC_SHARP_LOWLEVEL_H_

#include "stm32u3xx_hal.h"
#include "sharp.h"

/* Hardware initialization */
void SPI2_Init(void);
void TIM1_Init(void);

/* Error handling */
void LCD_Error_Handler(void) __attribute__((noreturn));

/* LCD operations */
void __lcd_init(void);
void LCD_write_line(uint8_t *buf);
void lcd_refresh(void);
void delay_us(uint16_t us);
void lcd_keep_alive(void);
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim1;

#endif /* INC_SHARP_LOWLEVEL_H_ */
