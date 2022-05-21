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
#include "http.h"
#include "comms.h"
#include "mqtt.h"

static const char *TAG = "COMMS";

static char jwt[512];

void comms_init(const TCommsSetup *const commsSetup)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta(commsSetup->wifiSsid, commsSetup->wifiPass);

    ESP_LOGI(TAG, "ESP_HTTP_MODE_TCP");
    void http_init();
    http_get_jwt(commsSetup->userEmail, commsSetup->userPass, commsSetup->userId, jwt);

    if (*(commsSetup->deviceId) == NULL)
    {
        // register device
        http_reg_device(jwt, commsSetup->deviceId);
    }

    ESP_LOGI(TAG, "ESP_MQTT_MODE_TCP");
    mqtt_init(jwt, *(commsSetup->userId), *(commsSetup->deviceId), commsSetup->userEmail);
}