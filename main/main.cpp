#include <cstdint>
#include <cstdio>
#include <sys/unistd.h>
#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "soc/gpio_num.h"
#include "esp_task.h"
#include "esp_log.h"
#include "bdc_motor.h"
#include "pid_ctrl.h"

static const char *TAG = "dc-motor";

namespace pin {
constexpr gpio_num_t led = GPIO_NUM_12;
constexpr gpio_num_t motor_pwm_a = GPIO_NUM_32;
constexpr gpio_num_t motor_pwm_b = GPIO_NUM_33;
}

namespace constants {
    constexpr uint32_t timer_resolution = 10e6;
    constexpr uint32_t pwm_freq = 10e3;
}

/*void blink_task(void *params) {
    while (true) {
        gpio_set_level(pin::led, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        gpio_set_level(pin::led, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}*/

extern "C" void app_main(void)
{
    gpio_output_enable(pin::led);

    ESP_LOGI(TAG, "Create DC motor");
    bdc_motor_config_t motor_config = {
        .pwma_gpio_num = pin::motor_pwm_a,
        .pwmb_gpio_num = pin::motor_pwm_b,
        .pwm_freq_hz = constants::pwm_freq,
    };
    bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = 0,
        .resolution_hz = constants::timer_resolution,
    };

    bdc_motor_handle_t motor = nullptr;
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor_config, &mcpwm_config, &motor));

    bdc_motor_enable(motor);
    bdc_motor_forward(motor);
    // xTaskCreate(&blink_task, "blink-task", 2048, nullptr, 0, nullptr);

    int32_t inc = 100;
    uint32_t current_speed = 500;
    bool dir = true;
    bdc_motor_set_speed(motor, current_speed);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        current_speed += inc;
        if (current_speed == 1000) {
            inc = -inc;
        }
        
        if (current_speed == 500) {
            inc = -inc;
            dir = !dir;
            if (dir) {
                bdc_motor_forward(motor);
            } else {
                bdc_motor_reverse(motor);
            }
        }
        ESP_LOGI(TAG, "%i", current_speed);
        bdc_motor_set_speed(motor, current_speed);
    }
}
