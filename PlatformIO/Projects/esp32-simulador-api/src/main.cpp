#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ================= Configurações de Pinos e PWM =================
const int PINO_VERMELHO = 22;
const int PINO_VERDE    = 23;
const int PINO_AMARELO  = 21;
const int PINO_BUZZER   = 18;

const int CANAL_VERMELHO = 0;
const int CANAL_VERDE    = 1;
const int CANAL_AMARELO  = 2;
const int FREQ_PWM       = 5000;
const int RESOLUCAO_PWM  = 8;

// ================= Configurações de Rede =================
const char* ssid = "JOSE";
const char* password = "19Do26.,";

// Endereço HTTPS público da sua API
const char* serverUrl = "https://api-irrigacao.online/api/telemetry";

// ================= Definição dos Estados do Sistema =================
enum EstadoSistema {
  ESTADO_NORMAL,       // Tudo OK, Wi-Fi OK -> Verde Respirando
  ESTADO_DEGRADADO,    // Sem Wi-Fi / Armazenando -> Verde e Amarelo Respirando
  ESTADO_FALHA_PARCIAL,// Erro em sensor -> Vermelho Pisca + Relatório
  ESTADO_CRITICO       // +10 min em erro -> 3 LEDs piscam + Alarme
};

EstadoSistema estadoAtual = ESTADO_NORMAL;

// Timers e Controle Assíncrono (millis)
unsigned long tempoAnteriorLeitura = 0;
unsigned long tempoInicioErroCritico = 0;
const unsigned long INTERVALO_LEITURA = 10000; // Coleta a cada 10 segundos
const unsigned long TEMPO_LIMITE_CRITICO = 600000; // 10 minutos (600.000 ms)
bool erroCriticoAtivo = false;

// ================= Fila de Armazenamento Offline =================
// Estrutura para guardar a leitura na RAM
struct DadosSensor {
  float umidade;
  float temperatura;
  float pressao;
  float irradiacao_solar;
  String status_hardware;
};

const int MAX_BUFFER = 60; // 60 leituras de 10s = 10 minutos de histórico
DadosSensor filaOffline[MAX_BUFFER];
int qtdNaFila = 0;

// ================= Funções Auxiliares de Som e LED =================
void emitirSom(int frequencia, int duracaoMs) {
  tone(PINO_BUZZER, frequencia, duracaoMs);
}

void desligarTodosLEDs() {
  ledcWrite(CANAL_VERMELHO, 0);
  ledcWrite(CANAL_VERDE, 0);
  ledcWrite(CANAL_AMARELO, 0);
}

void piscarCurto(int pinoCanal, int frequenciaSom) {
  ledcWrite(pinoCanal, 255);
  emitirSom(frequenciaSom, 80);
  delay(100);
  ledcWrite(pinoCanal, 0);
  delay(100);
}

// ================= 1. POST: Autodiagnóstico no Boot =================
void executarPowerOnSelfTest() {
  Serial.println("\n--- INICIANDO POST (Power-On Self Test) ---");

  // Fase 1: Teste Visual e Sonoro Geral
  for (int i = 0; i < 4; i++) {
    ledcWrite(CANAL_VERMELHO, 255);
    ledcWrite(CANAL_VERDE, 255);
    ledcWrite(CANAL_AMARELO, 255);
    emitirSom(1500, 100);
    delay(150);
    desligarTodosLEDs();
    delay(150);
  }

  // Fase 2: Teste de Varredura de Sensores Simulados
  const char* sensores[] = {"Umidade_Solo", "Temperatura", "Pressao", "Irradiacao_Solar"};
  ledcWrite(CANAL_VERDE, 255);
  
  for (int i = 0; i < 4; i++) {
    Serial.printf("Testando hardware/sensor [%s]... OK\n", sensores[i]);
    ledcWrite(CANAL_VERDE, 0);
    emitirSom(2000 + (i * 300), 60);
    delay(80);
    ledcWrite(CANAL_VERDE, 255);
    delay(300);
  }
  desligarTodosLEDs();

  // Fase 3: Teste de Conectividade Wi-Fi
  Serial.print("Conectando ao Wi-Fi para validação...");
  WiFi.begin(ssid, password);
  
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 15) {
    piscarCurto(CANAL_AMARELO, 800);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi e Rota de Rede validados!");
    emitirSom(2500, 200);
    estadoAtual = ESTADO_NORMAL;
  } else {
    Serial.println("\nWi-Fi indisponível. Entrando em Modo Contingência.");
    emitirSom(400, 400);
    estadoAtual = ESTADO_DEGRADADO;
    tempoInicioErroCritico = millis();
  }
  Serial.println("--- DIAGNÓSTICO CONCLUÍDO ---\n");
}

// ================= 2. Motor Visual Assíncrono (Sem Delay) =================
void atualizarLuzesEfeitos() {
  unsigned long tempo = millis();
  int brilhoRespiracao = (exp(sin(tempo / 1000.0 * M_PI)) - 0.36787944) * 108.0;

  switch (estadoAtual) {
    case ESTADO_NORMAL:
      ledcWrite(CANAL_VERDE, brilhoRespiracao);
      ledcWrite(CANAL_AMARELO, 0);
      ledcWrite(CANAL_VERMELHO, 0);
      break;

    case ESTADO_DEGRADADO:
      ledcWrite(CANAL_VERDE, brilhoRespiracao);
      ledcWrite(CANAL_AMARELO, brilhoRespiracao);
      ledcWrite(CANAL_VERMELHO, 0);
      break;

    case ESTADO_FALHA_PARCIAL:
      ledcWrite(CANAL_VERDE, brilhoRespiracao);
      ledcWrite(CANAL_AMARELO, 0);
      ledcWrite(CANAL_VERMELHO, (tempo / 300) % 2 == 0 ? 255 : 0);
      break;

    case ESTADO_CRITICO:
      bool estadoPisca = (tempo / 200) % 2 == 0;
      ledcWrite(CANAL_VERDE, estadoPisca ? 255 : 0);
      ledcWrite(CANAL_AMARELO, estadoPisca ? 255 : 0);
      ledcWrite(CANAL_VERMELHO, estadoPisca ? 255 : 0);
      
      if (estadoPisca && (tempo % 1000 < 200)) {
        emitirSom(3500, 50);
      }
      break;
  }
}

// ================= 3. Simulação e Coleta de Dados =================
void processarEEnviarLeituras() {
  // Simulação dos valores lidos
  DadosSensor leituraAtual;
  leituraAtual.umidade          = random(350, 750) / 10.0;
  leituraAtual.temperatura      = random(200, 350) / 10.0;
  leituraAtual.pressao          = random(1000, 1025);
  leituraAtual.irradiacao_solar = random(2000, 10000) / 10.0;
  bool falhaSimulada            = (random(0, 100) < 15);

  leituraAtual.status_hardware = falhaSimulada ? "ERRO_SENSOR_TEMPERATURA_FALHA_LEITURA" : "OK";

  Serial.println("\n-------------------------------------------");
  Serial.printf("Leitura: Umid=%.1f%% | Temp=%.1f°C | Press=%.0fhPa | Irrad=%.1f W/m²\n", 
                leituraAtual.umidade, leituraAtual.temperatura, leituraAtual.pressao, leituraAtual.irradiacao_solar);

  if (falhaSimulada) {
    Serial.println("[ALERTA HARDWARE]: Falha detectada!");
    estadoAtual = ESTADO_FALHA_PARCIAL;
    emitirSom(500, 300);
  }

  // --- ADICIONA À FILA DE MEMÓRIA ---
  if (qtdNaFila < MAX_BUFFER) {
    filaOffline[qtdNaFila] = leituraAtual;
    qtdNaFila++;
  } else {
    // Fila cheia: desloca todos os itens para a esquerda (descarta o mais antigo)
    for (int i = 1; i < MAX_BUFFER; i++) {
      filaOffline[i-1] = filaOffline[i];
    }
    filaOffline[MAX_BUFFER - 1] = leituraAtual; // Guarda o mais novo no fim
    Serial.println("[AVISO]: Fila cheia! Descartando registro mais antigo.");
  }

  Serial.printf("Registros na fila de envio: %d/%d\n", qtdNaFila, MAX_BUFFER);

  // --- TENTA ENVIAR TUDO O QUE ESTÁ NA FILA ---
  if (WiFi.status() == WL_CONNECTED) {
    JsonDocument doc; // O ArduinoJson 7 gerencia a memória automaticamente
    doc["sensor_id"] = "esp32_multisensor_01";
    
    JsonArray leiturasJson = doc["leituras"].to<JsonArray>();
    
    // Converte toda a fila em um único JSON em lote
    for (int i = 0; i < qtdNaFila; i++) {
      JsonObject obj = leiturasJson.add<JsonObject>();
      obj["umidade"]          = filaOffline[i].umidade;
      obj["temperatura"]      = filaOffline[i].temperatura;
      obj["pressao"]          = filaOffline[i].pressao;
      obj["irradiacao_solar"] = filaOffline[i].irradiacao_solar;
      obj["status_hardware"]  = filaOffline[i].status_hardware;
    }

    String jsonBody;
    serializeJson(doc, jsonBody);

    // Conexão e envio seguro
    WiFiClientSecure client;
    client.setInsecure(); 

    HTTPClient http;
    http.setTimeout(5000); // TRAVA DE SEGURANÇA: Limite de 5 segundos
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    ledcWrite(CANAL_AMARELO, 255);
    int httpCode = http.POST(jsonBody);
    ledcWrite(CANAL_AMARELO, 0);

    if (httpCode > 0) {
      if (httpCode == 200) {
        Serial.printf("[HTTP 200]: Sucesso! %d registro(s) entregues.\n", qtdNaFila);
        
        ledcWrite(CANAL_VERDE, 255);
        emitirSom(2000, 50);
        delay(60);
        emitirSom(3000, 80);
        delay(100);
        
        // Limpa a fila e zera alertas, pois tudo foi entregue!
        qtdNaFila = 0;
        tempoInicioErroCritico = 0;
        erroCriticoAtivo = false;
        if (!falhaSimulada) estadoAtual = ESTADO_NORMAL; 
      } else {
        Serial.printf("[ERRO API %d]: Servidor rejeitou os dados. Retendo na fila...\n", httpCode);
        if (!falhaSimulada) estadoAtual = ESTADO_DEGRADADO;
        if (tempoInicioErroCritico == 0) tempoInicioErroCritico = millis();
      }
    } else {
      Serial.printf("[ERRO REDE]: Sem resposta (%s). Retendo na fila...\n", http.errorToString(httpCode).c_str());
      if (!falhaSimulada) estadoAtual = ESTADO_DEGRADADO;
      if (tempoInicioErroCritico == 0) tempoInicioErroCritico = millis();
    }
    http.end();
  } else {
    Serial.println("[OFFLINE]: Sem Wi-Fi. Dados retidos na fila de memória RAM.");
    if (!falhaSimulada) estadoAtual = ESTADO_DEGRADADO;
    if (tempoInicioErroCritico == 0) tempoInicioErroCritico = millis();
  }

  // --- VERIFICAÇÃO DO TEMPO LIMITE DE 10 MINUTOS ---
  if (tempoInicioErroCritico > 0 && (millis() - tempoInicioErroCritico >= TEMPO_LIMITE_CRITICO)) {
    estadoAtual = ESTADO_CRITICO;
    Serial.println("[CRÍTICO]: Sistema isolado ou falhando há mais de 10 minutos!");
  }
}

// ================= Setup e Loop Principal =================
void setup() {
  Serial.begin(115200);

  ledcSetup(CANAL_VERMELHO, FREQ_PWM, RESOLUCAO_PWM);
  ledcSetup(CANAL_VERDE, FREQ_PWM, RESOLUCAO_PWM);
  ledcSetup(CANAL_AMARELO, FREQ_PWM, RESOLUCAO_PWM);

  ledcAttachPin(PINO_VERMELHO, CANAL_VERMELHO);
  ledcAttachPin(PINO_VERDE, CANAL_VERDE);
  ledcAttachPin(PINO_AMARELO, CANAL_AMARELO);
  pinMode(PINO_BUZZER, OUTPUT);

  executarPowerOnSelfTest();
}

void loop() {
  atualizarLuzesEfeitos();

  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnteriorLeitura >= INTERVALO_LEITURA) {
    tempoAnteriorLeitura = tempoAtual;
    processarEEnviarLeituras();
  }
}