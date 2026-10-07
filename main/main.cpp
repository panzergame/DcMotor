#include <cstdio>
#include <sys/unistd.h>
#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "soc/gpio_num.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "bdc_motor.h"
#include "pid_ctrl.h"

static const char *TAG = "dc-motor";

namespace pin {
constexpr gpio_num_t led = GPIO_NUM_12;
constexpr gpio_num_t motor_pwm_a = GPIO_NUM_32;
constexpr gpio_num_t motor_pwm_b = GPIO_NUM_33;
constexpr gpio_num_t encoder_a = GPIO_NUM_34;
constexpr gpio_num_t encoder_b = GPIO_NUM_35;
}

namespace constants {
    constexpr uint32_t timer_resolution = 10e6;
    constexpr uint32_t pwm_freq = 10e3;
    constexpr int32_t pcnt_hight = 1000;
    constexpr int32_t pcnt_low = -1000;
}

/*void blink_task(void *params) {
    while (true) {
        gpio_set_level(pin::led, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        gpio_set_level(pin::led, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}*/

void pcnt_probe_task(void *params) {
    pcnt_unit_handle_t pcnt_unit = reinterpret_cast<pcnt_unit_handle_t>(params);

    int cur_pulse_count = -1;
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &cur_pulse_count));
    ESP_LOGI(TAG, "pulses: %i", cur_pulse_count);
}

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

    ESP_LOGI(TAG, "Init pcnt driver to decode rotary signal");
    pcnt_unit_config_t unit_config = {
        .group_id = 0,
        .clk_src = PCNT_CLK_SRC_APB,
        .low_limit = constants::pcnt_low,
        .high_limit = constants::pcnt_hight,
        .intr_priority = 0,
        .flags = {
            .accum_count = true
        }
    };
    pcnt_unit_handle_t pcnt_unit = nullptr;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = pin::encoder_a,
        .level_gpio_num = pin::encoder_b,
        .flags = {}
    };
    pcnt_channel_handle_t pcnt_chan_a = nullptr;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = pin::encoder_b,
        .level_gpio_num = pin::encoder_a,
        .flags = {}
    };
    pcnt_channel_handle_t pcnt_chan_b = nullptr;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, constants::pcnt_hight));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, constants::pcnt_low));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    const esp_timer_create_args_t timer_args {
        .callback = pcnt_probe_task,
        .arg = pcnt_unit,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "pcnt task",
        .skip_unhandled_events = true
    };

    esp_timer_handle_t timer = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 100e3));

    int32_t inc = 100;
    uint32_t current_speed = 800;
    bool dir = true;
    bdc_motor_set_speed(motor, current_speed);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        current_speed += inc;
        if (current_speed == 1000) {
            inc = -inc;
        }
        
        if (current_speed == 800) {
            inc = -inc;
            dir = !dir;
            if (dir) {
                bdc_motor_forward(motor);
            } else {
                bdc_motor_reverse(motor);
            }
        }
        // ESP_LOGI(TAG, "%i", current_speed);
        bdc_motor_set_speed(motor, current_speed);
    }
}
