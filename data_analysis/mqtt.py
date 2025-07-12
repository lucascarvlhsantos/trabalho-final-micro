import os, csv
from threading import Timer
from dotenv import load_dotenv
import paho.mqtt.client as mqtt
load_dotenv(os.path.join(os.path.dirname(__file__), "configs", ".env"))

topico_medidas = os.getenv("TOPICO_MEDIDAS")

class MqttClient:
    def __init__(self, broker_uri, broker_port, usuario, senha):
        self.mqttClient = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
        self.mqttClient.username_pw_set(usuario, senha)
        self.mqttClient.on_connect = self.on_connect
        self.mqttClient.on_message = self.on_message
        self.saidas = []
        self.contagem_started = False
        self.connect(broker_uri, int(broker_port))

    def connect(self, broker, porta):
        self.mqttClient.connect(broker, porta, keepalive=60)
        self.mqttClient.loop_forever()

    def on_connect(self, client, userdata, flags, status_code, properties):
        if status_code:
            print("Conectado com sucesso ao broker")
            self.mqttClient.subscribe(f"{topico_medidas}")
        else:
            print(f"Falha na conexão, código de retorno: {status_code}\n")

    def on_message(self, client, userdata, msg):
        tópico = msg.topic
        payload = msg.payload.decode('utf-8')
        self.converter_payload(payload.strip() + '\0')
    
    def gerarCSV(self):
        colunas = ["Saída atual", "RPS", "RPM"]

        with open("medidas.csv", mode="w", newline="", encoding="utf-8") as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=colunas)
            writer.writeheader()
            writer.writerows(self.saidas)

        print("CSV gerado: medidas.csv")

    def on_timer(self):
        print(self.saidas)

    def converter_payload(self, payload):
        dict = {}
        payload = payload.split('\0', 1)[0].strip()

        itens = payload.split(" | ")
        counter = 0
        for item in itens:
            chave, valor_bruto = item.split(":", 1)
            chave = chave.strip() 
            valor_bruto = valor_bruto.strip()

            match chave:
                case "Saída atual":
                    numero = int(valor_bruto.rstrip("%"))
                    if numero == 100 and (not self.contagem_started):
                        print("Iniciando contagem")
                        t = Timer(120.0, self.gerarCSV)
                        t.start()
                        self.contagem_started = True

                    counter += 1
                    
                case "RPS":
                    numero = int(valor_bruto.rstrip("Hz"))
                    counter += 1
                
                case "RPM":
                    numero = int(valor_bruto.rstrip("RPM").strip())
                    counter += 1
                
                case _:
                    continue
                
            dict[chave] = numero

            if (counter == 3):
                self.saidas.append(dict)
                dict = {}
                counter = 0

client = MqttClient(os.getenv("BROKER_URI"),      
                    os.getenv("BROKER_PORT"), 
                    os.getenv("BROKER_USERNAME"),
                    os.getenv("BROKER_PASSWORD"))

