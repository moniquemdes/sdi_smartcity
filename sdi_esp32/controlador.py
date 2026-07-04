import paho.mqtt.client as mqtt
import json

# IP do seu servidor na AWS
MQTT_BROKER = "3.131.158.173" 
TOPIC_PEDIDOS = "smartcity/cruzamento_1/pedidos"
TOPIC_COMANDOS = "smartcity/cruzamento_1/comandos"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("🧠 Controlador conectado à AWS! Escutando pedidos de decisão...")
        client.subscribe(TOPIC_PEDIDOS)
    else:
        print("Falha na conexão. Código:", rc)

def on_message(client, userdata, msg):
    try:
        # 1. Lê o JSON que chegou do ESP32
        payload = msg.payload.decode('utf-8')
        filas = json.loads(payload)
        
        # 2. LÓGICA DE DECISÃO: Encontra qual pista tem mais carros
        pista_escolhida = max(filas, key=filas.get)
        maior_fila = filas[pista_escolhida]
        
        # 3. Calcula o tempo de sinal verde
        if maior_fila == 0:
            # Se não houver nenhum carro, abre 'cima' por padrão por 2 segundos
            pista_escolhida = "cima"
            tempo_verde = 2
        else:
            # 2 segundos + 1 segundo para cada carro (máximo de 5 segundos)
            tempo_verde = min(2 + maior_fila, 5) 
            
        print(f"📊 Estado recebido: {filas} -> Decisão: Abrir '{pista_escolhida}' por {tempo_verde}s")
        
        # 4. Envia o comando de volta para a AWS (que repassa para o ESP32)
        mensagem_comando = f"{pista_escolhida},{tempo_verde}"
        client.publish(TOPIC_COMANDOS, mensagem_comando)
        
    except Exception as e:
        print(f"⚠️ Erro ao processar o JSON: {e}")

# Configuração do Cliente MQTT
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Inicia a conexão e mantém rodando para sempre
client.connect(MQTT_BROKER, 1883, 60)
client.loop_forever()
