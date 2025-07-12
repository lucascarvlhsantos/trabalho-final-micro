#define INTERVALO_CONTAGEM 1000 // em ms
#include "mqttHandler.h"

// DEFINIÇÃO DE PINOS
QueueHandle_t filaMedicoes;
const uint8_t acopladorPin = 25;
const uint8_t inputPin = 35;
unsigned long testeT0;

uint16_t rpmAtual = 0;
uint8_t rpsAtual = 0;
float porcentagemSaida = 0.5;
uint8_t contadorTeste = 0;

char outputBuffer[64];

void taskMedicoes(void* pvParameters) {
  uint8_t contagemMedidas = 0;
  uint8_t contagemCiclos = 0;
  uint8_t medidasRotacao[10];
  for (uint8_t i = 0; i < sizeof(medidasRotacao); i++) {
    medidasRotacao[i] = 0;
  }

  unsigned long t0 = millis();
  unsigned long mqttT0 = millis();
  while (true) {
    while(millis() - t0 < INTERVALO_CONTAGEM) {
      uint16_t input = analogRead(inputPin);
      //Serial.println(input);
      if (input < 500) {
        while(analogRead(inputPin) < 500);
        contagemMedidas += 1;

        //delay(1);
      }
    }

    medidasRotacao[contagemCiclos] = contagemMedidas;
    // calculo da média móvel
    uint16_t somaValoresRotacoes = 0;
    for (uint8_t i = 0; i < sizeof(medidasRotacao); i++) {
      somaValoresRotacoes += medidasRotacao[i];
    }

    rpsAtual = (uint8_t)round((float)somaValoresRotacoes / (float)sizeof(medidasRotacao));
    medidasRotacao[contagemCiclos] = rpsAtual;
    contagemMedidas = 0;
    contagemCiclos = (contagemCiclos + 1) % 10;

    if (millis() - mqttT0 > 2000) {
      sprintf(outputBuffer, "Saída atual: %u%% | RPS: %u Hz | RPM: %u RPM", uint8_t(porcentagemSaida * 100), rpsAtual, (uint16_t) rpsAtual * 60);
      xQueueSend(filaMedicoes, &outputBuffer, portMAX_DELAY);
      mqttT0 = millis();
    }

    t0 = millis();
    delay(1);
  }
}

void setup() {
  Serial.begin(115200);
  ledcAttach(acopladorPin, 5000, 12);
  ledcWrite(acopladorPin, 0);
  
  filaMedicoes = xQueueCreate(10, sizeof(outputBuffer));
  xTaskCreate(
    mqttTask,
    "taskMqtt",
    3 * 1024,
    NULL,
    3,
    NULL
  );
  xTaskCreate(
    taskMedicoes,
    "taskMedicoes",
    2 * 1024,
    NULL,
    1,
    NULL
  );

  testeT0 = millis();
}

void loop() {
  if (millis() - testeT0 > 10000) {
    contadorTeste += 1;
    testeT0 = millis();
  }

  if (contadorTeste > 4) {
    porcentagemSaida += 0.05;
    Serial.printf("Saida atual: %u\n", (uint16_t)round(porcentagemSaida * 4095));
    contadorTeste = 0;
    testeT0 = millis();
    return;
  }
  
  porcentagemSaida = constrain(porcentagemSaida, 0.50, 0.90);
  ledcWrite(acopladorPin, (uint16_t)round(porcentagemSaida * 4095));
  delay(2);
}