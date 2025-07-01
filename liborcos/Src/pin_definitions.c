#define DEFINE_PINS
#include "pin_definitions.h"

/* ---------------------- Pin Definitions ---------------------- */
// Define individual pins
#define DEFINE_PIN(name, port, pin_num) const gpio_pin_t name = {port, pin_num}

// Define pin arrays
#define DEFINE_PIN_ARRAY(name, ...) const gpio_pin_t name[] = {__VA_ARGS__}

/* Pin Definitions here */
DEFINE_PIN(display_cs, GPIOC, GPIO_PIN_0);
DEFINE_PIN(v_div, GPIOC, GPIO_PIN_7);
DEFINE_PIN(v_sens, GPIOC, GPIO_PIN_8);
DEFINE_PIN(disp, GPIOA, GPIO_PIN_10);
DEFINE_PIN(v5_en, GPIOA, GPIO_PIN_15);
DEFINE_PIN(extcomin, GPIOA, GPIO_PIN_9);
/* Note: If you change pin definitions below, make sure you update
 * HAL_NVIC_[Enable|Disable]IRQ calls throughout the code to match */
const gpio_pin_t column_pin_array[] = {
    {GPIOA, GPIO_PIN_0},
    {GPIOA, GPIO_PIN_2},
    {GPIOA, GPIO_PIN_3},
    {GPIOA, GPIO_PIN_4},
    {GPIOA, GPIO_PIN_5},
    {GPIOA, GPIO_PIN_8},
};
const size_t column_pin_array_count = 6; 

const gpio_pin_t row_pin_array[] = {
    {GPIOB, GPIO_PIN_0},
    {GPIOB, GPIO_PIN_1},
    {GPIOB, GPIO_PIN_2},
    {GPIOB, GPIO_PIN_3},
    {GPIOB, GPIO_PIN_4},
    {GPIOB, GPIO_PIN_5},
    {GPIOB, GPIO_PIN_13},
    {GPIOB, GPIO_PIN_14},
    {GPIOB, GPIO_PIN_15},
};
const size_t row_pin_array_count = 9;
