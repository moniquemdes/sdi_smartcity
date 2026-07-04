import paho.mqtt.client as mqtt
import time
import random

MQTT_BROKER = "3.131.158.173"
TOPIC_SIMULADOR = "smartcity/cruzamento_1/simulador"
PISTAS = ["cima", "baixo", "esquerda", "direita"]

client = mqtt.Client()
client.connect(MQTT_BROKER, 1883, 60)
client.loop_start()

print("🚀 Simulador Iniciado: Gerando tráfego pesado...")

try:
    while True:
        pista_sorteada = random.choice(PISTAS)
        client.publish(TOPIC_SIMULADOR, pista_sorteada)
        print(f"🚙 Carro gerado na pista: {pista_sorteada}")
        time.sleep(1) # Gera 1 carro por segundo
except KeyboardInterrupt:
    print("Simulador desligado.")
    client.loop_stop()
