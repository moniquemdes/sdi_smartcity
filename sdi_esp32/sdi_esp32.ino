#include <WiFi.h>
#include <PubSubClient.h>
#include <algorithm> 

// ================= CONFIGURAÇÕES =================
const char* ssid = "Carlos";       
const char* password = "deumaoito";   
const char* mqtt_server = "3.131.158.173"; 
const int mqtt_port = 1883;

const char* topico_simulador = "smartcity/cruzamento_1/simulador"; 
const char* topico_pedidos   = "smartcity/cruzamento_1/pedidos";   
const char* topico_comandos  = "smartcity/cruzamento_1/comandos";  
const char* topico_status    = "smartcity/cruzamento_1/status"; 

WiFiClient espClient;
PubSubClient client(espClient);

// ================= ESTADO DO CRUZAMENTO =================
int fila_cima = 0;
int fila_baixo = 0;
int fila_esquerda = 0;
int fila_direita = 0;

String pista_aberta = "nenhuma";
unsigned long tempoFimVerde = 0;
unsigned long tempoUltimaPassagem = 0;

unsigned long t_inicio_pedido = 0;
bool aguardando_decisao = false;

// ================= FUNÇÃO: DECISÃO NA BORDA =================
void realizarDecisaoBorda() {
    unsigned long t_inicio = micros();

    int max_fila = std::max({fila_cima, fila_baixo, fila_esquerda, fila_direita});
    
    String pista_edge = "cima";
    if (max_fila == fila_baixo) pista_edge = "baixo";
    else if (max_fila == fila_esquerda) pista_edge = "esquerda";
    else if (max_fila == fila_direita) pista_edge = "direita";
    
    int tempo_segundos = (max_fila == 0) ? 2 : std::min(2 + max_fila, 5);
    unsigned long latencia_local = micros() - t_inicio;
    
    // PUBLICA RESULTADO DA BORDA
    String msgBorda = "Decisao: " + pista_edge + " | Tempo: " + String(tempo_segundos) + "s | Latencia Processamento: " + String(latencia_local) + "micro";
    client.publish(topico_status, msgBorda.c_str());
    
    Serial.println("\n⚡ [BORDA] Decisão Local Tomada e enviada!");
}

// ================= CALLBACK MQTT =================
void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem = "";
  for (int i = 0; i < length; i++) mensagem += (char)payload[i];
  String topicoRecebido = String(topic);

  if (topicoRecebido == topico_simulador) {
    if (mensagem == "cima") fila_cima++;
    else if (mensagem == "baixo") fila_baixo++;
    else if (mensagem == "esquerda") fila_esquerda++;
    else if (mensagem == "direita") fila_direita++;
  }
  else if (topicoRecebido == topico_comandos) {
    unsigned long latencia = millis() - t_inicio_pedido;
    String msgLatencia = String(latencia) + " ms";
    client.publish("smartcity/cruzamento_1/latencia", msgLatencia.c_str());
    
    Serial.printf("\n☁️ [NUVEM] Decisão Recebida! Latência de Rede: %lu ms\n", latencia);
    
    int virgula = mensagem.indexOf(',');
    pista_aberta = mensagem.substring(0, virgula);
    int tempo_segundos = mensagem.substring(virgula + 1).toInt();
    
    tempoFimVerde = millis() + (tempo_segundos * 1000);
    aguardando_decisao = false;
  }
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32-Cruzamento-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(topico_simulador);
      client.subscribe(topico_comandos);
    } else { delay(2000); }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect(); 
  client.loop(); 

  unsigned long agora = millis();

  if (!aguardando_decisao) {
    if (agora > tempoFimVerde) {
      pista_aberta = "nenhuma";
      
      // 1. PRIMEIRO: Envia o pedido (Gera o cabeçalho no Monitor)
      String estadoFilas = "{\"cima\":" + String(fila_cima) + ",\"baixo\":" + String(fila_baixo) + 
                           ",\"esquerda\":" + String(fila_esquerda) + ",\"direita\":" + String(fila_direita) + "}";
      client.publish(topico_pedidos, estadoFilas.c_str());
      t_inicio_pedido = millis(); 
      aguardando_decisao = true;
      
      // 2. SEGUNDO: Realiza a decisão da Borda
      realizarDecisaoBorda();
    }
    else {
      // Passagem dos carros
      if (agora - tempoUltimaPassagem > 1000) {
        bool passouCarro = false;
        if (pista_aberta == "cima" && fila_cima > 0) { fila_cima--; passouCarro = true; }
        else if (pista_aberta == "baixo" && fila_baixo > 0) { fila_baixo--; passouCarro = true; }
        else if (pista_aberta == "esquerda" && fila_esquerda > 0) { fila_esquerda--; passouCarro = true; }
        else if (pista_aberta == "direita" && fila_direita > 0) { fila_direita--; passouCarro = true; }
        
        if (passouCarro) {
          Serial.printf("[-1] Carro passou em '%s'!\n", pista_aberta.c_str());
        }
        tempoUltimaPassagem = agora;
      }
    }
  }
}
