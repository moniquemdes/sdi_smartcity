#include <WiFi.h>
#include <PubSubClient.h>

// ================= CONFIGURAÇÕES =================
const char* ssid = "Gustavo";       // COLOQUE SEU WI-FI AQUI
const char* password = "teste1234";   // COLOQUE SUA SENHA AQUI

const char* mqtt_server = "3.131.158.173";    // O IP PÚBLICO DA SUA AWS
const int mqtt_port = 1883;
const char* topico = "smartcity/joselindo";

// ================= OBJETOS =======================
WiFiClient espClient;
PubSubClient client(espClient);

int contadorCarros = 0;
unsigned long ultimoTempo = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando na rede: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado! IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Fica em loop até reconectar ao MQTT
  while (!client.connected()) {
    Serial.print("Tentando conexao MQTT com a Nuvem...");
    // Cria um ID de cliente aleatório
    String clientId = "ESP32-SmartCity-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado na AWS!");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando de novo em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Simula a passagem de um carro a cada 5 segundos
  if (millis() - ultimoTempo > 5000) {
    ultimoTempo = millis();
    contadorCarros++;

    // Cria a mensagem em formato JSON simples
    String mensagem = "{\"sensor\": \"ESP32_01\", \"status\": \"carro_detectado\", \"total\": " + String(contadorCarros) + "}";
    
    // Envia (Publica) a mensagem no tópico MQTT
    client.publish(topico, mensagem.c_str());
    
    Serial.print("Mensagem enviada para a Nuvem: ");
    Serial.println(mensagem);
  }
}