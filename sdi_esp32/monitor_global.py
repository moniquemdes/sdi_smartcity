import paho.mqtt.client as mqtt

MQTT_BROKER = "3.131.158.173"
# O símbolo '#' faz o script ouvir TODOS os tópicos do cruzamento de uma vez!
TOPIC_ALL = "smartcity/cruzamento_1/#" 

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Monitor Global Conectado à Nuvem AWS!")
        client.subscribe(TOPIC_ALL)
        print(f"🎧 Escutando a rede no canal: {TOPIC_ALL}\n")
        print("=" * 50)
    else:
        print("Falha na conexão.")

def on_message(client, userdata, msg):
    topico = msg.topic
    payload = msg.payload.decode('utf-8')
    
    # 1. Carros chegando (Deixei comentado para não poluir muito a tela, 
    if "simulador" in topico:
        pass 
        # print(f"🚙 [SIMULADOR] enviou carro para -> {payload}")
        
    # 2. O ESP32 pedindo uma decisão
    elif "pedidos" in topico:
        print(f"\n📡 [ESP32] Solicitando decisão! Filas: {payload}")
        
    # 3. O Controlador na Nuvem mandando a resposta
    elif "comandos" in topico:
        dados = payload.split(',')
        pista = dados[0]
        tempo = dados[1]
        print(f"🧠 [NUVEM] Respondeu: Abrir '{pista}' por {tempo}s")
        
    # 4. O ESP32 devolvendo o cálculo da latência!
    elif "latencia" in topico:
        print(f"⏱️  [ESP32] CÁLCULO DE LATÊNCIA: A mensagem demorou {payload} para ir e voltar!")
        print("=" * 50)
        
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("🔄 Iniciando Monitor Global...")
client.connect(MQTT_BROKER, 1883, 60)
client.loop_forever()
