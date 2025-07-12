#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H
#include "freertos/queue.h"

extern QueueHandle_t filaMedicoes;
void mqttTask(void* pvParameters);
#endif