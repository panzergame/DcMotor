#include <cstdio>
#include <sys/unistd.h>
#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "soc/gpio_num.h"
#include "esp_task.h"
#include "esp_log.h"
#include "pid_ctrl.h"

namespace pin {
constexpr gpio_num_t led = GPIO_NUM_12;
}

void blink_task(void *params) {
    while (true) {
        gpio_set_level(pin::led, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        gpio_set_level(pin::led, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

extern "C" void app_main(void)
{
    gpio_output_enable(pin::led);

    xTaskCreate(&blink_task, "blink-task", 2048, nullptr, 0, nullptr);

    /*while (true) {
        gpio_set_level(pin::led, 0);
        sleep(1);
        gpio_set_level(pin::led, 1);
        sleep(1);
    }*/
}
