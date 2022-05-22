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
