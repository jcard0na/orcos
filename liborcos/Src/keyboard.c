#include "keyboard.h"
#include "main.h"
#include "orcos.h"
#include "pin_definitions.h"
#include "sharp.h" // For LCD functions
#include "sharp_lowlevel.h" // For delay_us()
#include "SEGGER_RTT.h"

#define KEY_QUEUE_SIZE 16
static uint16_t key_queue[KEY_QUEUE_SIZE];
static uint8_t key_queue_head = 0;
static uint8_t key_queue_tail = 0;

enum KeyState
{
  WAIT_PRESS,
  WAIT_RELEASE_AFTER_PRESS
};

enum KeyState key_state = WAIT_PRESS;

void key_push(uint16_t keycode)
{
  uint8_t next_head = (key_queue_head + 1) % KEY_QUEUE_SIZE;
  if (next_head != key_queue_tail)
  {
    key_queue[key_queue_head] = keycode;
    key_queue_head = next_head;
  }
}

uint16_t key_pop(void)
{
  if (key_queue_head == key_queue_tail)
  {
    return 0; // Queue empty
  }
  uint16_t key = key_queue[key_queue_tail];
  key_queue_tail = (key_queue_tail + 1) % KEY_QUEUE_SIZE;
  DEBUG_PRINT("K-POP: %d:%d\n", key >> 8, key & 0xff);
  return key;
}

void wait_for_key_press()
{
  int last_keycode = 0;
  while (1)
  {
    sys_sleep(0);
    uint16_t keycode = scan_keyboard();

    // DEBUG_PRINT("key_state: %d keycode: %d\n", key_state, keycode);
    switch (key_state)
    {
    case WAIT_PRESS:
      if (keycode > 0)
      {
        last_keycode = keycode;
        key_state = WAIT_RELEASE_AFTER_PRESS;
      }
      break;
    case WAIT_RELEASE_AFTER_PRESS:
      if (keycode == 0)
      {
        key_push(last_keycode);
        key_state = WAIT_PRESS;
        return;
      }
      break;
    }
  }
}

uint16_t scan_keyboard(void)
{
  uint16_t key1 = 0;
  uint16_t key2 = 0;
  uint8_t key_count = 0;

  // Set all output column pins to high
  for (uint16_t column = 0; column < NUM_COLUMN_PINS; column++) {
    GPIO_WRITE(column_pin_array[column], GPIO_PIN_SET);
  }

  // Scan columns
  for (int16_t column = 0; column < NUM_COLUMN_PINS; column++) {
    GPIO_WRITE(column_pin_array[column], GPIO_PIN_RESET); // Set column pin to low
    delay_us(20); // Small delay for transitions

    // Read row pins
    for (int16_t row = 0; row < NUM_ROW_PINS; row++) {
      if ((GPIO_READ(row_pin_array[row])) == GPIO_PIN_RESET) {
        uint16_t keycode = (column + row * NUM_COLUMN_PINS + 1);
        if (key_count == 0) {
          key1 = keycode;
          key_count = 1;
        } else if (key_count == 1) {
          key2 = keycode;
          key_count = 2;
        } else {
          // More than two keys pressed
          key_count = 3;
          break;
        }
      }
    }

    GPIO_WRITE(column_pin_array[column], GPIO_PIN_SET); // Reset column
    if (key_count > 2) break; // Early exit if too many keys
  }

  // Reset all output column pins to low
  for (uint16_t column = 0; column < NUM_COLUMN_PINS; column++) {
    GPIO_WRITE(column_pin_array[column], GPIO_PIN_RESET);
  }

  if (key_count > 0) {
    lcd_keep_alive();
  }

  if (key_count == 1) {
    return key1;
  } else if (key_count == 2) {
    return key1 | (key2 << 8); // Combine both keys
  } else if (key_count > 2) {
    return 0xFFFF; // Too many keys
  }
  return 0; // No keys pressed
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
