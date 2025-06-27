#include "keyboard.h"
#include "main.h"
#include "orcos.h"
#include "sharp.h"  // For LCD functions
#include "SEGGER_RTT.h"

const uint16_t row_pin_array[NUM_ROW_PINS] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
    GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};

const uint16_t column_pin_array[NUM_COLUMN_PINS] = {
    GPIO_PIN_0, GPIO_PIN_2, GPIO_PIN_3,
    GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_8};


uint16_t scan_keyboard(void)
{
  int16_t pressed_column = -1;
  int16_t pressed_row = -1;
  uint8_t multiple_key_press = 0;

  // Set all output column pins to high
  for (uint16_t column = 0; column < NUM_COLUMN_PINS; column++)
  {
    HAL_GPIO_WritePin(GPIOA, column_pin_array[column], GPIO_PIN_SET);
  }

  // Scan columns
  for (int16_t column = 0; column < NUM_COLUMN_PINS; column++)
  {
    HAL_GPIO_WritePin(GPIOA, column_pin_array[column], GPIO_PIN_RESET); // Set column pin to low
    delay_us(20);                                                       // Small delay (20 us) for all transitional processes to finish

    // Read row pins
    for (int16_t row = 0; row < NUM_ROW_PINS; row++)
    {
      GPIO_PinState status = HAL_GPIO_ReadPin(GPIOB, row_pin_array[row]);
      if (status == GPIO_PIN_RESET)
      {
        if (pressed_column == -1)
        {
          pressed_row = row;
          pressed_column = column;
        }
        else
        {
          // Another key press already registered
          multiple_key_press = 1;
        }
      }
    }

    HAL_GPIO_WritePin(GPIOA, column_pin_array[column], GPIO_PIN_SET); // Set column pin back to high
  }

  // Reset all output column pins to low
  for (uint16_t column = 0; column < NUM_COLUMN_PINS; column++)
  {
    HAL_GPIO_WritePin(GPIOA, column_pin_array[column], GPIO_PIN_RESET);
  }

  if (pressed_column >= 0) {
    lcd_keep_alive();
  }

  if (pressed_column >= 0 && !multiple_key_press)
    // Only if single key pressed
    return (pressed_column + pressed_row * NUM_COLUMN_PINS + 1);
  else
    // Return 0 if no keys or multiple keys pressed
    return 0;
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
  DEBUG_PRINT("INT Rising\n");
  HAL_ResumeTick();
  HAL_PWR_DisableSleepOnExit();
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  DEBUG_PRINT("INT Falling\n");
  HAL_ResumeTick();
  HAL_PWR_DisableSleepOnExit();
}
