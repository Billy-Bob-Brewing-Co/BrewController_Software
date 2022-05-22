/*
 * Copyright 2020 Robert Carey
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
#include "comms.h"
#include "config.h"
#include "brew.h"

char *wifiSsid, *wifiPass;
char *userEmail, *userPass, *userId;
char *deviceId;

esp_err_t err;

static const char *TAG = "MAIN";

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
    // Initialize NVS
    //  printf("btn lvl: %d\n", gpio_get_level(GPIO_BTN_RST));
    //  if (gpio_get_level(GPIO_BTN_RST))
    //  {
    //  nvs_flash_erase();
    //    gpio_set_level(GPIO_LED_STS, true);
    //  }
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
    // Initialize NVS (needed for wifi)
    initNvs();

    brew_init();

    TCommsSetup commsSetup;
    commsSetup.wifiSsid = wifiSsid;
    commsSetup.wifiPass = wifiPass;

    comms_init(&commsSetup);
}
