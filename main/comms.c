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

#include "wifi.h"
#include "comms.h"

static const char *TAG = "COMMS";

void comms_init(const TCommsSetup *const commsSetup)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta(commsSetup->wifiSsid, commsSetup->wifiPass);
}