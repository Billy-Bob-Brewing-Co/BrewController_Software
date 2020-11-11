/* DESCRIPTION ***************************************************

 File:                brew.c

 Author:              Robert Carey

 Creation Date:       11th November 2020

 Description:         

 END DESCRIPTION ***************************************************************/

#include "brew.h"
#include "esp_log.h"

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(BREW_EVENTS);

void brew_init(void)
{
  esp_event_loop_args_t brew_task_args = {
      .queue_size = 5,
      .task_name = "brew_task", // task will be created
      .task_priority = 5,
      .task_stack_size = 3072,
      .task_core_id = 1,
  };

  ESP_ERROR_CHECK(esp_event_loop_create(&brew_task_args, &BREW_TASK));
}