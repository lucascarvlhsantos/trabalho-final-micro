#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include "configs.h"
#include "mqttHandler.h"

struct MqttConfig {
  char* broker;
  uint16_t porta;
  char* username;
  char* password;
};

WiFiClient   espClient;
PubSubClient client(espClient);

int8_t conectarAoWifi(const char* ssid, const char* password) {
  Serial.printf("Conectando à rede: %s\n", ssid);
  WiFi.begin(ssid, password);

  uint32_t t0 = millis();
  uint8_t quantidadeDePontos = 1;
  while (WiFi.status() != WL_CONNECTED) {
    if (quantidadeDePontos < 10)
      Serial.print(".");
    
    else {
      Serial.print("\n.");
      quantidadeDePontos = 1;
    }
    if (millis() - t0 > 10000) {
      Serial.printf("Não foi possível conectar à: %s\n", ssid);
      return -1;
    } 

    delay(500);
  }

  Serial.printf("Conectado à: %s\n", ssid);
  return 0;
}

int8_t conectarAoBroker(MqttConfig* config) {
  client.setServer(config->broker, config->porta);

  Serial.printf("Conectando ao Broker MQTT.\n");
  delay(1);
  if (client.connected()) {
    Serial.printf("Conectado ao Broker MQTT.\n");
    delay(1);
    return 0;
  }
  
  if (client.connect("espClient", config->username, config->password)) {
    delay(10);
    return 0;
  }
  
  return -1;
}

void publicar(const char* topico, const char* payload) {
  Serial.println(payload);
  client.beginPublish(topico, strlen(payload), false);
  client.print(payload);
  client.endPublish();
}

void mqttTask(void* pvParameters) {
  if(conectarAoWifi(WIFI_SSID, WIFI_PASSWORD) < 0){
    vTaskDelete(NULL);

    delay(10);
    return;
  }

  MqttConfig mqttConfig = {
    .broker   = BROKER_URI,
    .porta    = BROKER_PORT,
    .username = BROKER_USERNAME,
    .password = BROKER_PASSWORD
  };

  uint32_t t0 = millis();
  while(conectarAoBroker(&mqttConfig) < 0) {
    if (millis() - t0 > 15000) {
      Serial.println("Não foi possível conectar ao Broker");
      vTaskDelete(NULL);
      return;
    }
  
    delay(20);
  }

  Serial.printf("Conectado ao Broker MQTT.\n");
  client.subscribe("controle/setpoint");

  float rotacoes = 0;
  char queueReceiveBuffer[64];
  while (true) {
    while (!client.loop()) {
      conectarAoBroker(&mqttConfig);

      delay(500);
    }

    if (xQueueReceive(filaMedicoes, &queueReceiveBuffer, portMAX_DELAY) == pdTRUE) {
      publicar("lucascarvlhsantos@gmail.com/motor/medidas", queueReceiveBuffer);
    }

    delay(10);
  }
}