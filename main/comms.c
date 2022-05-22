/* DESCRIPTION ***************************************************

 File:                comms.c

 Author:              Robert Carey

 Creation Date:       9th November 2020

 Description:

 END DESCRIPTION ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "wifi.h"
#include "comms.h"

static const char *TAG = "COMMS";

void comms_init(const TCommsSetup *const commsSetup)
{
    /* Needed for the wifi module */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta(commsSetup->wifiSsid, commsSetup->wifiPass);
}