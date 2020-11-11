/* DESCRIPTION ***************************************************

 File:                brew.h

 Author:              Robert Carey

 Creation Date:       11th November 2020

 Description:         

 END DESCRIPTION ***************************************************************/

#ifndef MAIN_BREW_H_
#define MAIN_BREW_H_

#include "esp_event.h"
// #include "event_source.h"
#include "esp_event_base.h"

esp_event_loop_handle_t BREW_TASK;

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
  uint32_t sensors[3];
} brewStatus_t;

void brew_init(void);

#endif /* MAIN_BREW_H_ */
