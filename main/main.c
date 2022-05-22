/*
 * Copyright 2020 Robert Carey
 */

#include <stdio.h>
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "comms.h"
#include "config.h"
#include "brew.h"

esp_err_t err;

static const char *TAG = "MAIN";

void app_main()
{
    brew_init();

    TCommsSetup commsSetup;
    commsSetup.wifiSsid = CONFIG_ESP_WIFI_SSID;
    commsSetup.wifiPass = CONFIG_ESP_WIFI_PASSWORD;

    comms_init(&commsSetup);
}
