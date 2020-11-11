/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "string.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"
#include "comms.h"
#include "config.h"
#include "brew.h"

#define GPIO_DS18B20_AMB 26
#define GPIO_DS18B20_BEER 27
#define MAX_DEVICES 8
#define DS18B20_RESOLUTION (DS18B20_RESOLUTION_12_BIT)
#define SAMPLE_PERIOD (1000) // milliseconds
#define FRIDGEGPIO 33
#define HEATPADGPIO 32
#define RED_LED_GPIO 21
#define YELLOW_LED_GPIO 22

char *wifiSsid, *wifiPass;
char *userEmail, *userPass, *userId;
char *deviceId;

esp_err_t err;

static const char *TAG = "MAIN";

float temp_setpoint = 20.0;
const float temp_hysteresis = 0.5;
float ambTempVal = 20.0;
float beerTempVal = 20.0;

static void nvsString(nvs_handle_t handle, char *key, char **val)
{
  size_t reqSize;
  err = nvs_get_str(handle, key, NULL, &reqSize);
  if (err != ESP_OK)
  {
    // *val = NULL;
  }
  else
  {
    *val = malloc(reqSize);
    nvs_get_str(handle, key, *val, &reqSize);
  }
}

static void nvsStringProv(nvs_handle_t handle, char *key, char **val)
{
  size_t reqSize;
  err = nvs_get_str(handle, key, NULL, &reqSize);
  if (err != ESP_OK)
  {
    char line[128];

    int count = 0;
    printf("Please enter default value for %s\n", key);
    while (count < 128)
    {
      int c = fgetc(stdin);
      if (c == '\n')
      {
        line[count] = '\0';
        break;
      }
      else if (c > 0 && c < 127)
      {
        line[count] = c;
        ++count;
      }
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    err = nvs_set_str(handle, key, line);
    *val = malloc(sizeof(line));
    strcpy(*val, line);
    printf("%s : %s\n", key, line);
  }
  else
  {
    *val = malloc(reqSize);
    nvs_get_str(handle, key, *val, &reqSize);
  }
}

static esp_err_t initNvs(void)
{
  //Initialize NVS
  // printf("btn lvl: %d\n", gpio_get_level(GPIO_BTN_RST));
  // if (gpio_get_level(GPIO_BTN_RST))
  // {
  nvs_flash_erase();
  //   gpio_set_level(GPIO_LED_STS, true);
  // }
  err = nvs_flash_init();
  if (err != ESP_OK)
  {
    printf("Error %s\n", esp_err_to_name(err));
  }

  err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // get handle
  nvs_handle_t my_handle;
  err = nvs_open(NVS_PART_KEY, NVS_READWRITE, &my_handle); // currently using the default partition
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle\n", esp_err_to_name(err));
    return ESP_FAIL;
  }
  else
  {
    // Read
    // Wifi SSID
    nvsStringProv(my_handle, NVS_KEY_WIFI_SSID, &wifiSsid);

    // Wifi Password
    nvsStringProv(my_handle, NVS_KEY_WIFI_PASS, &wifiPass);

    // User Email
    nvsStringProv(my_handle, NVS_KEY_USER_EMAIL, &userEmail);

    // User Password
    nvsStringProv(my_handle, NVS_KEY_USER_PASS, &userPass);

    // User Id
    nvsString(my_handle, NVS_KEY_USER_ID, &userId);

    // device Id
    nvsString(my_handle, NVS_KEY_DEVICE_ID, &deviceId);

    err = nvs_commit(my_handle);
    if (err == ESP_OK)
    {
      ESP_LOGI(TAG, "NVS Success\n");
    }
    else
    {
      ESP_LOGE(TAG, "NVS Failed\n");
    }

    // Close
    nvs_close(my_handle);
  }
  nvs_stats_t nvs_stats;
  nvs_get_stats(NULL, &nvs_stats);
  printf("Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n",
         nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);

  return ESP_OK;
}

void app_main()
{
  //Initialize NVS (needed for wifi)
  initNvs();

  brew_init();

  TCommsSetup commsSetup;
  commsSetup.wifiSsid = wifiSsid;
  commsSetup.wifiPass = wifiPass;
  commsSetup.userEmail = userEmail;
  commsSetup.userPass = userPass;
  commsSetup.userId = &userId;
  commsSetup.deviceId = &deviceId;

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
    ds18b20_convert_and_read_temp(device_amb, &ambTempVal);
    ds18b20_convert_and_read_temp(device_beer, &beerTempVal);
    printf("Ambient Temp: %f\n", ambTempVal);
    printf("Beer Temp: %f\n", beerTempVal);

    if (ambTempVal > temp_setpoint)
    {
      // Beer may need cooling
      // If beer is warmer than setpoint by hysteresis value and fridge is off, turn fridge on
      if ((beerTempVal > (temp_setpoint + temp_hysteresis)) && !gpio_get_level(FRIDGEGPIO))
      {
        gpio_set_level(FRIDGEGPIO, 1);
      }
      // Otherwise, if beer is colder than setpoint by hysteresis value and fridge is on, turn fridge off
      else if ((beerTempVal < (temp_setpoint - temp_hysteresis)) && gpio_get_level(FRIDGEGPIO))
      {
        gpio_set_level(FRIDGEGPIO, 0);
      }
    }
    if (ambTempVal < temp_setpoint)
    {
      // Beer may need heating
      // If beer is warmer than setpoint by hysteresis value and heater is on, turn heater off
      if ((beerTempVal > (temp_setpoint + temp_hysteresis)) && gpio_get_level(FRIDGEGPIO))
      {
        gpio_set_level(FRIDGEGPIO, 0);
      }
      // Otherwise, if beer is colder than setpoint by hysteresis value and heater is off, turn heater on
      else if ((beerTempVal < (temp_setpoint - temp_hysteresis)) && !gpio_get_level(FRIDGEGPIO))
      {
        gpio_set_level(FRIDGEGPIO, 1);
      }
    }
  }
}
