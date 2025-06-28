#include "main.h"
#include "orcos.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  bool shift = false;
  static int counter = 0;
  orcos_init();

  LCD_power_off(1);

  while (1)
  {
    uint16_t keycode = 0;

    // blocking call -> wait for any key
    wait_for_key_press();
    keycode = key_pop();

    switch (keycode)
    {
    case KEY_F:
      shift = true;
      break;
    case KEY_SIGN:
      if (shift)
        LCD_test_screen(++counter);
      break;
    case KEY_0:
      if (shift)
        LCD_test_screen(--counter);
      break;
    case KEY_ON:
      if (shift)
      {
        LCD_power_off(1);
        // also blocking call -> wait for ON
        sys_sleep(1);
      }
      if (!LCD_is_on())
      {
        LCD_power_on();
      }
      break;
    }
    // Any key other keycode clear 'shift'
    if (keycode != KEY_NONE && keycode != KEY_F)
      shift = 0;
  }
}
