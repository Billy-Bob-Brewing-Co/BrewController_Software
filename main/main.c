/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"
#include "comms.h"

#define GPIO_DS18B20_AMB 26
#define GPIO_DS18B20_BEER 27
#define MAX_DEVICES 8
#define DS18B20_RESOLUTION (DS18B20_RESOLUTION_12_BIT)
#define SAMPLE_PERIOD (1000) // milliseconds
#define FRIDGEGPIO 33
#define HEATPADGPIO 32
#define RED_LED_GPIO 21
#define YELLOW_LED_GPIO 22

void app_main()
{
  //Initialize NVS (needed for wifi)
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  TCommsSetup commsSetup;
  commsSetup.wifiSsid = "Jar";
  commsSetup.wifiPass = "sourpickle";

  comms_init(&commsSetup);

  vTaskDelay(2000.0 / portTICK_PERIOD_MS);
  OneWireBus *AmbTemp;
  OneWireBus *BeerTemp;
  owb_rmt_driver_info rmt_driver_info_amb;
  owb_rmt_driver_info rmt_driver_info_beer;
  AmbTemp = owb_rmt_initialize(&rmt_driver_info_amb, GPIO_DS18B20_AMB, RMT_CHANNEL_1, RMT_CHANNEL_2);
  BeerTemp = owb_rmt_initialize(&rmt_driver_info_beer, GPIO_DS18B20_BEER, RMT_CHANNEL_3, RMT_CHANNEL_4);
  owb_use_crc(AmbTemp, true);
  owb_use_crc(BeerTemp, true);
  OneWireBus_SearchState search_state_amb = {0};
  OneWireBus_SearchState search_state_beer = {0};
  bool found_amb = false;
  bool found_beer = false;
  owb_search_first(AmbTemp, &search_state_amb, &found_amb);
  owb_search_first(BeerTemp, &search_state_beer, &found_beer);
  char rom_code_s_amb[17];
  char rom_code_s_beer[17];
  owb_string_from_rom_code(search_state_amb.rom_code, rom_code_s_amb, sizeof(rom_code_s_amb));
  owb_string_from_rom_code(search_state_beer.rom_code, rom_code_s_beer, sizeof(rom_code_s_beer));
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
    printf("Turning on the fridge\n");
    gpio_set_level(FRIDGEGPIO, 1);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Turning off the fridge\n");
    gpio_set_level(FRIDGEGPIO, 0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Turning on the heatpad\n");
    gpio_set_level(HEATPADGPIO, 1);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Turning off the heatpad\n");
    gpio_set_level(HEATPADGPIO, 0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Turning on red LED\n");
    gpio_set_level(RED_LED_GPIO, 1);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Turning off red LED\n");
    gpio_set_level(RED_LED_GPIO, 0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Turning on yellow LED\n");
    gpio_set_level(YELLOW_LED_GPIO, 1);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Turning off yellow LED\n");
    gpio_set_level(YELLOW_LED_GPIO, 0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    float ambTempVal, beerTempVal;
    ds18b20_convert_and_read_temp(device_amb, &ambTempVal);
    ds18b20_convert_and_read_temp(device_beer, &beerTempVal);
    printf("Ambient Temp: %f\n", ambTempVal);
    printf("Beer Temp: %f\n", beerTempVal);
  }
}
