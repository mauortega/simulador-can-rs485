#include "driver/gpio.h"
#include "esp_rom_sys.h"

#define STATUS_LED_GPIO GPIO_NUM_2
#define BLINK_DELAY_US 250000

void app_main(void) {
  gpio_reset_pin(STATUS_LED_GPIO);
  gpio_set_direction(STATUS_LED_GPIO, GPIO_MODE_OUTPUT);

  while (1) {
    gpio_set_level(STATUS_LED_GPIO, 1);
    esp_rom_delay_us(BLINK_DELAY_US);

    gpio_set_level(STATUS_LED_GPIO, 0);
    esp_rom_delay_us(BLINK_DELAY_US);
  }
}
