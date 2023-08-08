/*
 * Copyright 2020 Robert Carey
 */

#include "brew.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "time.h"
#include "string.h"

#include "onewire_bus.h"
#include "onewire_bus_rmt.h"
#include "ds18b20.h"
#include "cJSON.h"

#include "config.h"
#include "comms.h"

esp_event_loop_handle_t BREW_TASK;

static const char *TAG = "BREW";

/* Time in milliseconds at which the temp is sampled and state updated */
#define SAMPLE_PERIOD (2000)

#define TEMPERATURE_HYSTERESIS (0.5)

static time_t last_log_time;

onewire_bus_handle_t ambient_temp_handle;
onewire_bus_handle_t beer_temp_handle;

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(BREW_EVENTS);

static void brew_init_temp_sensors(void)
{
    onewire_rmt_config_t config = {
        .max_rx_bytes = 10, // 10 tx bytes(1byte ROM command + 8byte ROM number + 1byte device command)
    };

    /* Initialise Ambient temp bus */
    config.gpio_pin = PIN_TEMP_AMBIENT;
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&config, &ambient_temp_handle));
    ESP_LOGI(TAG, "Ambient temp 1-wire bus installed");

    /* Skip ROM id to address  all devices on the bus */
    (void)ds18b20_set_resolution(ambient_temp_handle, NULL, DS18B20_RESOLUTION_12B);
    (void)ds18b20_trigger_temperature_conversion(ambient_temp_handle, NULL);

    /* Initialise Beer temp bus */
    config.gpio_pin = PIN_TEMP_BEER;
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&config, &beer_temp_handle));
    ESP_LOGI(TAG, "Ambient temp 1-wire bus installed");

    /* Skip ROM id to address  all devices on the bus */
    (void)ds18b20_set_resolution(beer_temp_handle, NULL, DS18B20_RESOLUTION_12B);
    (void)ds18b20_trigger_temperature_conversion(ambient_temp_handle, NULL);
}

static void brew_init_peripherals(void)
{
    /* Configure GPIOs */
    esp_rom_gpio_pad_select_gpio(PIN_RELAY_FRIDGE);
    gpio_set_direction(PIN_RELAY_FRIDGE, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(PIN_RELAY_HEATBELT);
    gpio_set_direction(PIN_RELAY_HEATBELT, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(PIN_LED_RED);
    gpio_set_direction(PIN_LED_RED, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(PIN_LED_YELLOW);
    gpio_set_direction(PIN_LED_YELLOW, GPIO_MODE_OUTPUT);

    brew_init_temp_sensors();
}

/**
 * Read the temperature from the one wire bus. Currently only supports one device on the bus.
 *
 * @param handle Handle for the one wire bus
 *
 * @return Temperature in degrees celsius
 */
static float brew_get_temp(onewire_bus_handle_t handle)
{
    float temperature = 0;

    esp_err_t err = ds18b20_get_temperature(handle, NULL, &temperature);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read temperature, err: %d\n", err);
        return 0;
    }

    return temperature;
}

static void brew_log_data(float ambient_temp, float beer_temp)
{
    time_t now;
    time(&now);

    /* Only log to brefather every 15min */
    if ( now > (last_log_time + (CONFIG_LOG_PERIOD*60)))
    {
        last_log_time = now;

        cJSON *post_data = cJSON_CreateObject();

        cJSON_AddStringToObject(post_data, "name", CONFIG_DEVICE_NAME);
        cJSON_AddNumberToObject(post_data, "temp", beer_temp);
        cJSON_AddNumberToObject(post_data, "aux_temp", ambient_temp);
        cJSON_AddStringToObject(post_data, "temp_unit", "C");

        ESP_LOGI(TAG, "Data:\n%s", cJSON_Print(post_data));

        comms_send_logs(cJSON_Print(post_data), strlen(cJSON_Print(post_data)));

        cJSON_Delete(post_data);
    }
}

static void brew_main_task(void *pvParameters)
{
    while (1)
    {
        float ambient_temp = brew_get_temp(ambient_temp_handle);
        float beer_temp = brew_get_temp(beer_temp_handle);

        ESP_LOGI(TAG, "Beer: %.2fC, Ambient: %.2FC\n", beer_temp, ambient_temp);

        /* Decide if we should heat or cool the beer */
        if (beer_temp > CONFIG_BEER_TEMP_SETPOINT)
        {
            gpio_set_level(PIN_RELAY_HEATBELT, 0);
            if (beer_temp > (CONFIG_BEER_TEMP_SETPOINT + TEMPERATURE_HYSTERESIS))
            {
                ESP_LOGI(TAG, "Turn on fridge\n");
                gpio_set_level(PIN_RELAY_FRIDGE, 1);
            }
        }
        else if (beer_temp < CONFIG_BEER_TEMP_SETPOINT)
        {
            gpio_set_level(PIN_RELAY_FRIDGE, 0);
            if (beer_temp < (CONFIG_BEER_TEMP_SETPOINT - TEMPERATURE_HYSTERESIS))
            {
                ESP_LOGI(TAG, "Turn on heat\n");
                gpio_set_level(PIN_RELAY_HEATBELT, 1);
            }
        }

        brew_log_data(ambient_temp, beer_temp);

        vTaskDelay(SAMPLE_PERIOD / portTICK_PERIOD_MS);
    }
}

void brew_init(void)
{
    time(&last_log_time);

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
