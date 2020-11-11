/* DESCRIPTION ***************************************************

 File:                mqtt.c

 Author:              Robert Carey

 Creation Date:       11th November 2020

 Description:         

 END DESCRIPTION ***************************************************************/

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt.h"
#include "cJSON.h"

static const char *TAG = "MQTT";

static bool MQTTConnected = false;
esp_mqtt_client_handle_t Client;

static char UserId[40];
static char DeviceId[40];

static char *Jwt;

static void _mkTopic(char *custLevel, char *topic)
{
  strcpy(topic, UserId);
  strcat(topic, "/");
  strcat(topic, DeviceId);
  strcat(topic, "/");
  strcat(topic, custLevel);
}

enum cmd
{
  CMD_MANUAL,
  CMD_HOME
};

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
  esp_mqtt_client_handle_t client = event->client;
  char topic[100];
  int msgId;
  // your_context_t *context = event->context;
  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    MQTTConnected = true;
    _mkTopic("General", topic);
    msgId = esp_mqtt_client_subscribe(client, topic, 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msgId);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    MQTTConnected = false;
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    break;
  case MQTT_EVENT_DATA:
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);
    break;
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
  return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
  mqtt_event_handler_cb(event_data);
}

void mqtt_init(const char *jwt, const char *userId, const char *deviceId, const char *email)
{
  strcpy(UserId, userId);
  strcpy(DeviceId, deviceId);
  Jwt = jwt;

  esp_mqtt_client_config_t mqtt_cnfg = {
      .uri = "mqtt://10.1.5.39",
      .port = 8883,
      .username = email,
      .password = jwt,
  };

  Client = esp_mqtt_client_init(&mqtt_cnfg);
  esp_mqtt_client_register_event(Client, ESP_EVENT_ANY_ID, mqtt_event_handler, Client);
  esp_mqtt_client_start(Client);
}