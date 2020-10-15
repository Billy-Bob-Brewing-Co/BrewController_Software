/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define FRIDGEGPIO 33
#define HEATPADGPIO 32

void app_main()
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(FRIDGEGPIO);
    gpio_pad_select_gpio(HEATPADGPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(FRIDGEGPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(HEATPADGPIO, GPIO_MODE_OUTPUT);
    while(1)
    {
        /* Blink off (output low) */
	    printf("Turning off the fridge\n");
        gpio_set_level(FRIDGEGPIO, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        printf("Turning on the heatpad\n");
        gpio_set_level(HEATPADGPIO, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        printf("Turning off the heatpad\n");
        gpio_set_level(HEATPADGPIO, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
	    printf("Turning on the fridge\n");
        gpio_set_level(FRIDGEGPIO, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
