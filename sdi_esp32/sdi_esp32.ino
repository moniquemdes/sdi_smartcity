#include <WiFi.h>
#include <PubSubClient.h>

// ================= CONFIGURAÇÕES =================
const char* ssid = "Carlos";       
const char* password = "xxxx";   

const char* mqtt_server = "3.131.158.173"; // IP da AWS ou do seu Nó de Borda local
const int mqtt_port = 1883;

// Tópicos Arquitetura Distribuída
const char* topico_simulador = "smartcity/cruzamento_1/simulador"; 
const char* topico_pedidos   = "smartcity/cruzamento_1/pedidos";   
const char* topico_comandos  = "smartcity/cruzamento_1/comandos";  

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

// ================= MEDIÇÃO DE LATÊNCIA =================
unsigned long t_inicio_pedido = 0;
bool aguardando_decisao = false;

// ================= FUNÇÃO DE RECEPÇÃO MQTT =================
void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem = "";
  for (int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  String topicoRecebido = String(topic);

  // 1. CHEGOU UM CARRO DO SIMULADOR
  if (topicoRecebido == topico_simulador) {
    if (mensagem == "cima") fila_cima++;
    else if (mensagem == "baixo") fila_baixo++;
    else if (mensagem == "esquerda") fila_esquerda++;
    else if (mensagem == "direita") fila_direita++;
  }
  
  // 2. CHEGOU UMA DECISÃO DO CONTROLADOR
  else if (topicoRecebido == topico_comandos) {
    unsigned long latencia = millis() - t_inicio_pedido; // <--- CÁLCULO DA LATÊNCIA AQUI
    
    String msgLatencia = String(latencia) + " ms";
    client.publish("smartcity/cruzamento_1/latencia", msgLatencia.c_str());
    
    Serial.println("\n=======================================================");
    Serial.printf("DECISÃO RECEBIDA! Tempo de Latência: %lu ms\n", latencia);
    
    // Decodifica a ordem recebida (exemplo: "cima,5")
    int virgula = mensagem.indexOf(',');
    pista_aberta = mensagem.substring(0, virgula);
    int tempo_segundos = mensagem.substring(virgula + 1).toInt();
    
    // Atualiza o cronômetro do verde
    tempoFimVerde = millis() + (tempo_segundos * 1000);
    aguardando_decisao = false;
    
    Serial.printf("ABRINDO SINALEIRO: '%s' por %d segundos\n", pista_aberta.c_str(), tempo_segundos);
    Serial.println("=======================================================\n");
  }
}

void setup_wifi() {
  delay(1000);
  Serial.print("\nConectando a: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi conectado!");
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32-Estado-Cruzamento-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(topico_simulador); // Ouve os carros chegando
      client.subscribe(topico_comandos);  // Ouve o algoritmo decidindo
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop(); 

  unsigned long agora = millis();

  if (!aguardando_decisao) {
    // SE O SINAL VERDE ACABOU (ou no início do programa)
    if (agora > tempoFimVerde) {
      
      // 1. FECHA O SINALEIRO
      pista_aberta = "nenhuma";   
      Serial.println("\nTEMPO ESGOTADO! Fechando todos os sinaleiros.");
      
      // Monta o JSON de estado físico para enviar ao Controlador
      String estadoFilas = "{\"cima\":" + String(fila_cima) + ",\"baixo\":" + String(fila_baixo) + 
                           ",\"esquerda\":" + String(fila_esquerda) + ",\"direita\":" + String(fila_direita) + "}";
      
      Serial.println("Solicitando decisão ao Controlador e iniciando cronômetro...");
      client.publish(topico_pedidos, estadoFilas.c_str());
      
      // 2. DISPARA O CRONÔMETRO DE LATÊNCIA
      t_inicio_pedido = millis(); 
      aguardando_decisao = true;
    } 
    // SE O SINAL ESTÁ VERDE (Carros atravessando a rua)
    else {
      // Libera 1 carro por segundo da pista que estiver aberta
      if (agora - tempoUltimaPassagem > 1000) {
        bool passouCarro = false;
        
        if (pista_aberta == "cima" && fila_cima > 0) { fila_cima--; passouCarro = true; }
        else if (pista_aberta == "baixo" && fila_baixo > 0) { fila_baixo--; passouCarro = true; }
        else if (pista_aberta == "esquerda" && fila_esquerda > 0) { fila_esquerda--; passouCarro = true; }
        else if (pista_aberta == "direita" && fila_direita > 0) { fila_direita--; passouCarro = true; }
        
        if (passouCarro) {
          Serial.printf("[-1] Carro passou em '%s'! Filas -> C:%d | B:%d | E:%d | D:%d\n", pista_aberta.c_str(), fila_cima, fila_baixo, fila_esquerda, fila_direita);
        }
        tempoUltimaPassagem = agora;
      }
    }
  }
}
