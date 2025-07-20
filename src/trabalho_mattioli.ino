const uint16_t INTERVALO_CONTAGEM = 3000; // em ms
#include "mqttHandler.h"

// DEFINIÇÃO DE PINOS
const uint8_t acopladorPin = 25;
const uint8_t inputPin = 35;

uint16_t rpmAtual = 0;
float rpsAtual = 0;
float porcentagemSaida = 0.5;
uint8_t contadorTeste = 0;

unsigned long testeT0;
QueueHandle_t filaMedicoes;
char outputBuffer[64];

void tarefaMotor(void* pvParameters) {
  uint8_t contagemMedidas = 0;
  uint8_t contagemCiclos = 0;
  uint8_t medidasRotacao[10];
  for (uint8_t i = 0; i < sizeof(medidasRotacao); i++) {
    medidasRotacao[i] = 0;
  }

  unsigned long mqttT0 = millis();
  while (true) {
    unsigned long t0 = millis();
    while(millis() - t0 < INTERVALO_CONTAGEM) {
      uint16_t input = analogRead(inputPin);
      if (input < 500) {
        contagemMedidas += 1;
        while(analogRead(inputPin) < 2000);
      }
    }

    medidasRotacao[contagemCiclos] = contagemMedidas;
    // calculo da média móvel
    uint16_t somaValoresRotacoes = 0;
    for (uint8_t i = 0; i < sizeof(medidasRotacao); i++) {
      somaValoresRotacoes += medidasRotacao[i];
    }

    //rpsAtual = (uint8_t)round((float)somaValoresRotacoes / (float)sizeof(medidasRotacao));
    rpsAtual = (float) contagemMedidas / 3.0;
    medidasRotacao[contagemCiclos] = rpsAtual;
    contagemMedidas = 0;
    contagemCiclos = (contagemCiclos + 1) % 10;

    if (millis() - mqttT0 > 2000) {
      sprintf(outputBuffer, "Saída atual: %u%% | RPS: %.2f Hz | RPM: %u RPM", uint8_t(porcentagemSaida * 100), rpsAtual, (uint16_t) rpsAtual * 60);
      xQueueSend(filaMedicoes, &outputBuffer, portMAX_DELAY);
      Serial.println(outputBuffer);
      mqttT0 = millis();
    }

    delay(10);
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
    tarefaMotor,
    "tarefaMotor",
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
