import paho.mqtt.client as mqtt

# ================= CONFIGURAÇÕES =================
MQTT_BROKER = "3.131.158.173"
# O '#' escuta todos os sub-tópicos de cruzamento_1
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
    
    # Início do bloco de comparação (Pedido recebido)
    if "pedidos" in topico:
        print(f"\n{'='*70}")
        print(f"📡 FILAS ATUAIS: {payload}")
        print("-" * 70)
        
    # Resultados (Alinhados para comparação fácil)
    elif "status" in topico:
        print(f"⚡ [BORDA]  {payload}")
        
    elif "comandos" in topico:
        print(f"🧠 [NUVEM]  Decisão: {payload}")
        
    elif "latencia" in topico:
        print(f"☁️ [NUVEM]  Latência: {payload}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("🔄 Iniciando Monitor Global...")
client.connect(MQTT_BROKER, 1883, 60)
client.loop_forever()
