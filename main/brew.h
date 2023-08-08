/*
 * Copyright 2020 Robert Carey
 */

#pragma once

#include "esp_event.h"
#include "esp_event_base.h"

ESP_EVENT_DECLARE_BASE(BREW_EVENTS); // declaration of the task events family

enum
{
    BREW_CMD_SETPOINT_EVENT, // raised during an iteration of the loop within the task
    BREW_CMD_BREW_EVENT,
    BREW_STATUS_EVENT,
};

typedef struct
{
    int brewing;
    int error;
    uint32_t setpoint;
    float sensors[3];
} brewStatus_t;

void brew_init(void);
