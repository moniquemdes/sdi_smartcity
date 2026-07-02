#include <WiFi.h>
#include <PubSubClient.h>

// ================= CONFIGURAÇÕES =================
const char* ssid = "Carlos_5G";       // COLOQUE SEU WI-FI AQUI
const char* password = "deumaoito";   // COLOQUE SUA SENHA AQUI

const char* mqtt_server = "3.131.158.173";    // O IP PÚBLICO DA SUA AWS
const int mqtt_port = 1883;

const char* topico_publicar = "smartcity/cruzamento_1/sensores"; 
const char* topico_assinar  = "smartcity/cruzamento_1/semaforo"; 

// ================= OBJETOS E VARIÁVEIS ===================
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long ultimoTempoCarro = 0;
unsigned long ultimoTempoPessoa = 0;

int contadorCarros = 0;
int contadorPessoas = 0;

// Variável nova: memoriza o estado atual do semáforo (inicia verde)
String estadoSemaforoCarros = "VERDE"; 

String categoriasVeiculo[4] = {"carro", "moto", "onibus", "caminhao"};

// ================= FUNÇÃO DE RECEBER ORDENS (EDGE -> ESP) =================
void callback(char* topic, byte* payload, unsigned int length) {
  String comando = "";
  for (int i = 0; i < length; i++) {
    comando += (char)payload[i];
  }
  
  Serial.print("ORDEM DO EDGE RECEBIDA: ");
  
  // Atualizamos a variável de memória junto com o print
  if (comando == "CARROS_VERDE") {
    estadoSemaforoCarros = "VERDE";
    Serial.println("Semáforo Veículos -> VERDE");
  } 
  else if (comando == "CARROS_AMARELO") {
    estadoSemaforoCarros = "AMARELO";
    Serial.println("Semáforo Veículos -> AMARELO");
  } 
  else if (comando == "CARROS_VERMELHO") {
    estadoSemaforoCarros = "VERMELHO";
    Serial.println("Semáforo Veículos -> VERMELHO");
  }
  else if (comando == "PEDESTRE_VERDE") {
    Serial.println("🚶 Semáforo Pedestres -> VERDE (Pode atravessar)");
  }
  else if (comando == "PEDESTRE_VERMELHO") {
    Serial.println("🚶 Semáforo Pedestres -> VERMELHO (Aguarde)");
  }
  else {
    Serial.println(comando);
  }
}

// ================= SETUP WI-FI =================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado! IP Local: ");
  Serial.println(WiFi.localIP());
}

// ================= RECONEXÃO MQTT =================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT do Nó de Borda...");
    String clientId = "ESP32-Semaforo-01-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado!");
      client.subscribe(topico_assinar); // Escuta as ordens de semáforo
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando em 5 seg");
      delay(5000);
    }
  }
}

// ================= SETUP PRINCIPAL =================
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0)); 

  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ================= LOOP PRINCIPAL =================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); 

  unsigned long tempoAtual = millis();

  // ---------------- 1. SENSOR DE CARROS (Simulação) ----------------
  int intervaloCarro = random(3000, 8000); 
  
  if (tempoAtual - ultimoTempoCarro > intervaloCarro) {
    ultimoTempoCarro = tempoAtual;
    
    if (estadoSemaforoCarros == "VERDE") {
      contadorCarros++;

      String categoria = categoriasVeiculo[random(0, 4)];
      int velocidade = random(20, 70); 

      String msgCarro = "{\"sensor\": \"ESP32_Trafego\", \"cruzamento\": 1, \"tipo\": \"veiculo\", \"categoria\": \"" + categoria + "\", \"velocidade_kmh\": " + String(velocidade) + "}";
      
      client.publish(topico_publicar, msgCarro.c_str());
      Serial.print("🚗 [SENSOR] -> Edge: ");
      Serial.println(msgCarro);
    }
  }

  // ---------------- 2. SENSOR DE PEDESTRES (Simulação) ----------------
  int intervaloPessoa = random(10000, 20000);

  // O pedestre continua podendo apertar o botão/chegar na calçada a qualquer momento
  if (tempoAtual - ultimoTempoPessoa > intervaloPessoa) {
    ultimoTempoPessoa = tempoAtual;
    contadorPessoas++;

    String msgPessoa = "{\"sensor\": \"ESP32_Botao\", \"cruzamento\": 1, \"tipo\": \"pedestre\", \"evento\": \"aguardando_travessia\", \"total_pessoas\": " + String(contadorPessoas) + "}";
    
    client.publish(topico_publicar, msgPessoa.c_str());
    Serial.print("🚶 [SENSOR] -> Edge: ");
    Serial.println(msgPessoa);
  }
}
