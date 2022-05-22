/*
 * Copyright 2020 Robert Carey
 */

#pragma once

typedef struct
{
    char *wifiSsid;
    char *wifiPass;
} TCommsSetup;

void comms_init(const TCommsSetup *const commsSetup);

esp_err_t comms_send_logs(const char *payload, size_t payload_len);
