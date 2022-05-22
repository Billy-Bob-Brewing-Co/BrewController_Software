/*
 * Copyright 2020 Robert Carey
 */

#include "brew.h"
#include "esp_log.h"
#include <freertos/task.h>
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#define GPIO_DS18B20_AMB 26
#define GPIO_DS18B20_BEER 27
#define MAX_DEVICES 8
#define DS18B20_RESOLUTION (DS18B20_RESOLUTION_12_BIT)
#define SAMPLE_PERIOD (1000) // milliseconds
#define FRIDGEGPIO 33
#define HEATPADGPIO 32
#define RED_LED_GPIO 21
#define YELLOW_LED_GPIO 22

static void vBrewTask(void *pvParameters);
static void vStatusTask(void *pvParameters);

const float temp_hysteresis = 0.5;

brewStatus_t Brew_Status;

// Assigned to pointers for readbility
float *beerTemp = &Brew_Status.sensors[0];
float *ambTemp = &Brew_Status.sensors[1];
float *setTemp = &Brew_Status.sensors[2];

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(BREW_EVENTS);

void brew_init(void)
{
    // Assign Default vals
    *beerTemp = 20.0;
    *ambTemp = 20.0;
    *setTemp = 20.0;
    Brew_Status.brewing = 1;
    Brew_Status.error = 0;

    esp_event_loop_args_t brew_task_args = {
        .queue_size = 5,
        .task_name = "brew_task", // task will be created
        .task_priority = 5,
        .task_stack_size = 3072,
        .task_core_id = 1,
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&brew_task_args, &BREW_TASK));

    xTaskCreate(&vBrewTask, "Brew_task_main", 4096, NULL, 5, NULL);
    xTaskCreate(&vStatusTask, "Brew_status_task", 1024, NULL, 4, NULL);
}

static void vBrewTask(void *pvParameters)
{
    OneWireBus *AmbTemp;
    OneWireBus *BeerTemp;
    owb_rmt_driver_info rmt_driver_info_amb;
    owb_rmt_driver_info rmt_driver_info_beer;
    AmbTemp = owb_rmt_initialize(&rmt_driver_info_amb, GPIO_DS18B20_AMB, RMT_CHANNEL_1, RMT_CHANNEL_2);
    BeerTemp = owb_rmt_initialize(&rmt_driver_info_beer, GPIO_DS18B20_BEER, RMT_CHANNEL_3, RMT_CHANNEL_4);
    owb_use_crc(AmbTemp, true);
    owb_use_crc(BeerTemp, true);
    DS18B20_Info *device_amb = 0;
    DS18B20_Info *device_beer = 0;

    DS18B20_Info *ds18b20_info_amb = ds18b20_malloc(); // heap allocation
    DS18B20_Info *ds18b20_info_beer = ds18b20_malloc();

    device_amb = ds18b20_info_amb;
    device_beer = ds18b20_info_beer;

    ds18b20_init_solo(ds18b20_info_amb, AmbTemp); // only one device on bus
    ds18b20_init_solo(ds18b20_info_beer, BeerTemp);

    ds18b20_use_crc(ds18b20_info_amb, true); // enable CRC check on all reads
    ds18b20_use_crc(ds18b20_info_beer, true);
    ds18b20_set_resolution(ds18b20_info_amb, DS18B20_RESOLUTION);
    ds18b20_set_resolution(ds18b20_info_beer, DS18B20_RESOLUTION);

    gpio_pad_select_gpio(FRIDGEGPIO);
    gpio_pad_select_gpio(HEATPADGPIO);
    gpio_pad_select_gpio(YELLOW_LED_GPIO);
    gpio_pad_select_gpio(YELLOW_LED_GPIO);
    gpio_set_direction(FRIDGEGPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(HEATPADGPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(YELLOW_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(YELLOW_LED_GPIO, GPIO_MODE_OUTPUT);
    while (1)
    {
        ds18b20_convert_and_read_temp(device_amb, ambTemp);
        ds18b20_convert_and_read_temp(device_beer, beerTemp);
        printf("Ambient Temp: %f\n", *ambTemp);
        printf("Beer Temp: %f\n", *beerTemp);

        // Beer may need cooling
        // If beer is warmer than setpoint by hysteresis value and fridge is off, turn fridge on
        if ((*beerTemp > (*setTemp + temp_hysteresis)))
        {
            gpio_set_level(HEATPADGPIO, 0);
            gpio_set_level(FRIDGEGPIO, 1);
        }
        // Otherwise, if beer is colder than setpoint by hysteresis value and fridge is on, turn fridge off
        else if ((*beerTemp < (*setTemp - temp_hysteresis)))
        {
            gpio_set_level(FRIDGEGPIO, 0);
            gpio_set_level(HEATPADGPIO, 1);
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void _updateStatus()
{
    ESP_ERROR_CHECK(esp_event_post_to(BREW_TASK, BREW_EVENTS, BREW_STATUS_EVENT, &Brew_Status, sizeof(Brew_Status), portMAX_DELAY));
}

// trigger status event so that MQTT message is sent
static void vStatusTask(void *pvParameters)
{
    // Inital delay of 10 sec to allow for the device to configure and connect
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    while (1)
    {
        _updateStatus();
        // approx 1 min delay
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}