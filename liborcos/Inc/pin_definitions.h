#pragma once
#include "stm32u3xx_hal.h"
#include <stddef.h>  // for size_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} gpio_pin_t;

// Single declaration macro
#define DECLARE_PIN(name) extern const gpio_pin_t name

// Array declaration macro
#define DECLARE_PIN_ARRAY(name) extern const gpio_pin_t name[]; \
                                extern const size_t name##_count

// GPIO Operations
#define GPIO_WRITE(pin_arg, state) HAL_GPIO_WritePin((pin_arg).port, (pin_arg).pin, (state))
#define GPIO_TOGGLE(pin_arg)      HAL_GPIO_TogglePin((pin_arg).port, (pin_arg).pin)
#define GPIO_READ(pin_arg)        HAL_GPIO_ReadPin((pin_arg).port, (pin_arg).pin)

// Initialization macros
#define GPIO_INIT_SINGLE(pin_arg, mode, pull, speed) \
    do { \
        GPIO_InitTypeDef GPIO_InitStruct = {0}; \
        GPIO_InitStruct.Pin = (pin_arg).pin; \
        GPIO_InitStruct.Mode = (mode); \
        GPIO_InitStruct.Pull = (pull); \
        GPIO_InitStruct.Speed = (speed); \
        HAL_GPIO_Init((pin_arg).port, &GPIO_InitStruct); \
    } while(0)

#define GPIO_INIT_ARRAY(pins, mode, pull, speed) \
    do { \
        GPIO_InitTypeDef GPIO_InitStruct = {0}; \
        GPIO_InitStruct.Mode = (mode); \
        GPIO_InitStruct.Pull = (pull); \
        GPIO_InitStruct.Speed = (speed); \
        for (size_t i = 0; i < (pins##_count); i++) { \
            GPIO_InitStruct.Pin = (pins)[i].pin; \
            HAL_GPIO_Init((pins)[i].port, &GPIO_InitStruct); \
        } \
    } while(0)

/* Pin Declarations here:  they need to match definitions in pin_definitions.c */
DECLARE_PIN(display_cs);
DECLARE_PIN(v_div);
DECLARE_PIN(v_sens);
DECLARE_PIN(disp);
DECLARE_PIN(v5_en);
DECLARE_PIN(extcomin);
DECLARE_PIN_ARRAY(column_pin_array);
DECLARE_PIN_ARRAY(row_pin_array);

#ifdef __cplusplus
}
#endif
