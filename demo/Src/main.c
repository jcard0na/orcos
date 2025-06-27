#include "main.h"
#include "orcos.h"

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  orcos_init();
  calc_init();

  while (1)
  {
    uint16_t keycode = 0;
    uint16_t last_keycode = 0;

    wait_for_key_press();
    keycode = key_pop();

    if (keycode == 54 && !LCD_is_on() && last_keycode == 0)
    { // Calculator was OFF and the ON button was pressed
      LCD_power_on();
      keycode = 0;
    }
    int ret = 1;

    if (LCD_is_on())
    {

      if (keycode)
      {
        ret = calc_on_key(keycode);
      }

      if (!ret)
      { // OFF command received
        LCD_power_off(1);
        sys_sleep(1);
      }
    }

    last_keycode = keycode;
  }
}