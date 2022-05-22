/*
 * Copyright 2020 Robert Carey
 */

#include "brew.h"
#include "esp_log.h"
#include "freertos/task.h"

#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#include "config.h"

/* Time in milliseconds at which the temp is sampled and state updated */
#define SAMPLE_PERIOD (2000)

#define TEMPERATURE_HYSTERESIS (0.5)

owb_rmt_driver_info ambient_temp_drv_info = {0};
owb_rmt_driver_info beer_temp_drv_info = {0};

DS18B20_Info *ambient_temp_sensor = NULL;
DS18B20_Info *beer_temp_sensor = NULL;

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(BREW_EVENTS);

static void brew_init_temp_sensors(void)
{
    /* Configure One Wire Bus */
    OneWireBus *ambient_temp_owb = owb_rmt_initialize(&ambient_temp_drv_info, PIN_TEMP_AMBIENT,
                                                      RMT_CHANNEL_1, RMT_CHANNEL_2);
    owb_use_crc(ambient_temp_owb, true);

    OneWireBus *beer_temp_owb = owb_rmt_initialize(&beer_temp_drv_info, PIN_TEMP_BEER,
                                                      RMT_CHANNEL_3, RMT_CHANNEL_4);
    owb_use_crc(beer_temp_owb, true);

    ambient_temp_sensor = ds18b20_malloc();
    ds18b20_init_solo(ambient_temp_sensor, ambient_temp_owb);
    ds18b20_use_crc(ambient_temp_sensor, true);
    ds18b20_set_resolution(ambient_temp_sensor, DS18B20_RESOLUTION_12_BIT);

    beer_temp_sensor = ds18b20_malloc();
    ds18b20_init_solo(beer_temp_sensor, beer_temp_owb);
    ds18b20_use_crc(beer_temp_sensor, true);
    ds18b20_set_resolution(beer_temp_sensor, DS18B20_RESOLUTION_12_BIT);
}

static void brew_init_peripherals(void)
{
    /* Configure GPIOs */
    gpio_pad_select_gpio(PIN_RELAY_FRIDGE);
    gpio_set_direction(PIN_RELAY_FRIDGE, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_RELAY_HEATBELT);
    gpio_set_direction(PIN_RELAY_HEATBELT, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_RED);
    gpio_set_direction(PIN_LED_RED, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(PIN_LED_YELLOW);
    gpio_set_direction(PIN_LED_YELLOW, GPIO_MODE_OUTPUT);

    brew_init_temp_sensors();
}

static float brew_get_temp(DS18B20_Info *sensor)
{
    float temperature = 0;
    ds18b20_convert_and_read_temp(sensor, &temperature);
    return temperature;
}

static void brew_main_task(void *pvParameters)
{
    while (1)
    {
        float ambient_temp = brew_get_temp(ambient_temp_sensor);
        float beer_temp = brew_get_temp(beer_temp_sensor);

        printf("Ambient Temp: %f\n", ambient_temp);
        printf("Beer Temp: %f\n", beer_temp);

        /* Decide if we should heat or cool the beer */
        if (beer_temp > CONFIG_BEER_TEMP_SETPOINT)
        {
            gpio_set_level(PIN_RELAY_HEATBELT, 0);
            if (beer_temp > (CONFIG_BEER_TEMP_SETPOINT + TEMPERATURE_HYSTERESIS))
            {
                gpio_set_level(PIN_RELAY_FRIDGE, 1);
            }
        }
        else if (beer_temp < CONFIG_BEER_TEMP_SETPOINT)
        {
            gpio_set_level(PIN_RELAY_FRIDGE, 0);
            if (beer_temp < (CONFIG_BEER_TEMP_SETPOINT - TEMPERATURE_HYSTERESIS))
            {
                gpio_set_level(PIN_RELAY_HEATBELT, 1);
            }
        }

        vTaskDelay(SAMPLE_PERIOD / portTICK_PERIOD_MS);
    }
}

void brew_init(void)
{
    brew_init_peripherals();

    esp_event_loop_args_t brew_task_args = {
        .queue_size = 5,
        .task_name = "brew_task", // task will be created
        .task_priority = 5,
        .task_stack_size = 3072,
        .task_core_id = 1,
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&brew_task_args, &BREW_TASK));

    xTaskCreate(&brew_main_task, "Brew_task_main", 4096, NULL, 5, NULL);
}
