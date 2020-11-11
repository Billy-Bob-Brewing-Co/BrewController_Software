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
#include "brew.h"

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

static void _statusEventHandler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
  brewStatus_t *Brew_Status = (brewStatus_t *)event_data;
  char topic[100];
  cJSON *payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "jwt", Jwt);

  _mkTopic("status", topic);

  cJSON *data = cJSON_CreateObject();
  cJSON_AddNumberToObject(data, "brewing", Brew_Status->brewing);
  cJSON_AddNumberToObject(data, "error", Brew_Status->error);
  cJSON_AddNumberToObject(data, "setpoint", Brew_Status->sensors[2]);

  cJSON *sensors = cJSON_CreateArray();
  // Fridge Temp
  cJSON *fridge = cJSON_CreateObject();
  cJSON_AddNumberToObject(fridge, "data", Brew_Status->sensors[0]);
  cJSON_AddItemToArray(sensors, fridge);
  // Beer Temp
  cJSON *beer = cJSON_CreateObject();
  cJSON_AddNumberToObject(beer, "data", Brew_Status->sensors[1]);
  cJSON_AddItemToArray(sensors, beer);
  // Setpoint Target
  cJSON *setpoint = cJSON_CreateObject();
  cJSON_AddNumberToObject(setpoint, "data", Brew_Status->sensors[2]);
  cJSON_AddItemToArray(sensors, setpoint);

  // Add sensor arrat to object
  cJSON_AddItemToObject(data, "sensors", sensors);

  cJSON_AddItemToObject(payload, "data", data);

  printf("DATA: %s\n", cJSON_Print(payload));
  esp_mqtt_client_publish(Client, topic, cJSON_Print(payload), 0, 1, 0);

  cJSON_Delete(payload); // Deletes the array and its contents
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
  ESP_ERROR_CHECK(esp_event_handler_register_with(BREW_TASK, BREW_EVENTS, BREW_STATUS_EVENT, _statusEventHandler, NULL));
}