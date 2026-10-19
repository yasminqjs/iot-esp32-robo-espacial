/*
  Firmware: Controle Remoto do Robô com ESP32-S3 + LED RGB + Sensores (DHT e Ultrassônico) + CallMeBot
  Envia temperatura e distância no WhatsApp ao pressionar o botão (ATIVO/INATIVO)
  e também monitora alertas automáticos de limite de temperatura e distância.
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DHT.h> // Necessário instalar a biblioteca "DHT sensor library" (Adafruit)

// ===== Configuração do Wi-Fi =====
const char* WIFI_SSID  = "yasminet";
const char* WIFI_SENHA = "111204yasmin";

// ===== Configuração do CallMeBot =====
const String CALLMEBOT_TELEFONE = "557191361473";
const String CALLMEBOT_APIKEY   = "6345560";

// ===== Pinos do Joystick (ADC: 1 a 20) =====
const int PIN_VRX = 1;
const int PIN_VRY = 2;

// ===== Pinos de Status e Botão =====
const int PIN_LED_VERDE    = 4;
const int PIN_LED_VERMELHO = 5;
const int PIN_BOTAO_PARADA = 6;

// ===== Pinos do LED RGB =====
const int PIN_RGB_R = 15;
const int PIN_RGB_G = 16;
const int PIN_RGB_B = 17;

// ===== Pinos e Configuração dos Sensores =====
const int PIN_DHT  = 7;
#define DHT_TIPO DHT11   // Altere para DHT22 se estiver usando o modelo branco
DHT dht(PIN_DHT, DHT_TIPO);

const int PIN_TRIG = 8;
const int PIN_ECHO = 9;

// ===== Limites para Alerta (Valores "X") =====
const float TEMP_LIMITE = 30.0; // Alerta se Temperatura for >= 30.0 °C
const float DIST_LIMITE = 20.0; // Alerta se Distância for < 20.0 cm

// Variáveis de Controle do Sistema
bool sistemaLigado = true;
String ultimoComando = "";

// Variáveis para temporização do pisca do LED RGB
unsigned long tempoAnteriorRGB = 0;
const long intervaloPisca = 200; // Pisca a cada 200 ms
bool estadoRGB = false;

// Variáveis para temporização e controle de alertas automáticos (Evita spam no WhatsApp)
unsigned long tempoAnteriorSensores = 0;
const long intervaloSensores = 2000; // Lê os sensores a cada 2 segundos
bool alertaTempEnviado = false;
bool alertaDistEnviado = false;

// ---------------------------------------------------------------
// Funções auxiliares
// ---------------------------------------------------------------

void setCorRGB(bool r, bool g, bool b) {
  digitalWrite(PIN_RGB_R, r ? HIGH : LOW);
  digitalWrite(PIN_RGB_G, g ? HIGH : LOW);
  digitalWrite(PIN_RGB_B, b ? HIGH : LOW);
}

// Mede a distância em centímetros com o sensor HC-SR04
float lerDistanciaCM() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Timeout de 30000 us (~5 metros max) para não travar o loop se o sensor desconectar
  long duracao = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duracao == 0) {
    return -1.0; // Leitura inválida ou fora de alcance
  }
  return (duracao * 0.0343) / 2.0;
}

// Monta o texto com os dados atuais de Temperatura e Distância para incluir nas mensagens
String obterResumoSensores() {
  float temp = dht.readTemperature();
  float dist = lerDistanciaCM();

  String textoTemp = isnan(temp) ? "Erro DHT" : (String(temp, 1) + "°C");
  String textoDist = (dist < 0)  ? "Fora de alcance" : (String(dist, 1) + " cm");

  String resumo = " | 🌡️ Temp: " + textoTemp + " | 📏 Dist: " + textoDist;

  // Se no momento do clique algum limite estiver ultrapassado, avisa junto!
  if (!isnan(temp) && temp >= TEMP_LIMITE) {
    resumo += " ⚠️(TEMP ALTA!)";
  }
  if (dist > 0 && dist < DIST_LIMITE) {
    resumo += " ⚠️(OBSTÁCULO PRÓXIMO!)";
  }

  return resumo;
}

// Codifica o texto para ir na URL (espaços, acentos, emojis etc.)
String urlEncode(const String& texto) {
  String codificado = "";
  const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < texto.length(); i++) {
    uint8_t c = (uint8_t)texto[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      codificado += (char)c;
    } else {
      codificado += '%';
      codificado += hex[c >> 4];
      codificado += hex[c & 0x0F];
    }
  }
  return codificado;
}

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Conectando ao Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWi-Fi conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[ERRO] Não foi possível conectar ao Wi-Fi.");
  }
}

// Envia a mensagem para o WhatsApp via CallMeBot
void enviarWhatsApp(const String& mensagem) {
  conectarWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERRO] Sem Wi-Fi, mensagem não enviada.");
    return;
  }

  String url = "https://api.callmebot.com/whatsapp.php?phone=" + CALLMEBOT_TELEFONE +
               "&apikey=" + CALLMEBOT_APIKEY +
               "&text=" + urlEncode(mensagem);

  WiFiClientSecure cliente;
  cliente.setInsecure();

  HTTPClient http;
  http.begin(cliente, url);
  http.setTimeout(10000);

  Serial.println("[WhatsApp] Enviando: " + mensagem);

  int codigo = http.GET();
  String resposta = http.getString();

  Serial.print("[WhatsApp] Código HTTP: ");
  Serial.println(codigo);
  Serial.print("[WhatsApp] Resposta: ");
  Serial.println(resposta);

  http.end();
}

// Monitora os sensores continuamente e envia alerta caso passem do limite X
void verificarSensores() {
  float temperatura = dht.readTemperature();
  float distancia = lerDistanciaCM();

  // 1. Verificação de Temperatura
  if (!isnan(temperatura)) {
    if (temperatura >= TEMP_LIMITE) {
      if (!alertaTempEnviado) {
        String msg = "🌡️ [ALERTA] Temperatura atingiu " + String(temperatura, 1) + 
                     "°C (Limite: " + String(TEMP_LIMITE, 1) + "°C) | 📏 Dist: " + 
                     (distancia > 0 ? String(distancia, 1) + " cm" : "Fora de alcance");
        Serial.println(msg);
        enviarWhatsApp(msg);
        alertaTempEnviado = true; // Trava para não enviar repetidamente
      }
    } else if (temperatura < (TEMP_LIMITE - 1.0)) {
      alertaTempEnviado = false; // Rearma quando esfriar 1°C
    }
  }

  // 2. Verificação de Distância (Ultrassônico)
  if (distancia > 0) {
    if (distancia < DIST_LIMITE) {
      if (!alertaDistEnviado) {
        String msg = "⚠️ [ALERTA] Obstáculo próximo a " + String(distancia, 1) + 
                     " cm (Limite: < " + String(DIST_LIMITE, 1) + " cm) | 🌡️ Temp: " + 
                     (isnan(temperatura) ? "Erro DHT" : String(temperatura, 1) + "°C");
        Serial.println(msg);
        enviarWhatsApp(msg);
        alertaDistEnviado = true; // Trava para não enviar repetidamente
      }
    } else if (distancia >= (DIST_LIMITE + 2.0)) {
      alertaDistEnviado = false; // Rearma quando o objeto se afastar 2 cm
    }
  }
}

// ---------------------------------------------------------------
// Setup
// ---------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BOTAO_PARADA, INPUT_PULLUP);

  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);

  // Inicialização dos pinos do Ultrassônico e DHT
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  dht.begin();

  // Estado Inicial: Robô ligado e pronto
  digitalWrite(PIN_LED_VERDE, HIGH);
  digitalWrite(PIN_LED_VERMELHO, LOW);
  setCorRGB(false, false, false);

  Serial.println("==========================================");
  Serial.println("  CONTROLE REMOTO INICIALIZADO COM SUCESSO");
  Serial.println("  Status: Robô Conectado (LED Verde Aceso)");
  Serial.println("==========================================");

  conectarWiFi();
  delay(1000); // Aguarda 1 segundo para o sensor DHT estabilizar a primeira leitura
  enviarWhatsApp("🤖 Robô ATIVO - controle inicializado" + obterResumoSensores());
}

// ---------------------------------------------------------------
// Loop
// ---------------------------------------------------------------

void loop() {
  // Leitura do Botão de Desligamento Remoto
  if (digitalRead(PIN_BOTAO_PARADA) == LOW) {
    delay(50); // Debounce simples
    if (digitalRead(PIN_BOTAO_PARADA) == LOW) {
      // Lê os sensores no exato momento em que o botão foi pressionado
      String dadosSensores = obterResumoSensores();

      if (sistemaLigado) {
        sistemaLigado = false;

        digitalWrite(PIN_LED_VERDE, LOW);
        digitalWrite(PIN_LED_VERMELHO, HIGH);
        setCorRGB(false, false, false);

        Serial.println("\n[ALERTA] Comando enviado: DESLIGAR");
        Serial.println("Status: Robô Desligado / Motores Parados (LED Vermelho Aceso)");
        Serial.println("Leitura atual" + dadosSensores + "\n");

        enviarWhatsApp("🔴 Robô INATIVO - motores parados" + dadosSensores);
      } else {
        sistemaLigado = true;
        ultimoComando = ""; // Força imprimir o comando atual ao religar

        digitalWrite(PIN_LED_VERDE, HIGH);
        digitalWrite(PIN_LED_VERMELHO, LOW);

        Serial.println("\n[INFO] Comando enviado: LIGAR");
        Serial.println("Status: Robô Conectado (LED Verde Aceso)");
        Serial.println("Leitura atual" + dadosSensores + "\n");

        enviarWhatsApp("🟢 Robô ATIVO - pronto para comandos" + dadosSensores);
      }

      // Aguarda a liberação do botão para evitar repetição contínua
      while (digitalRead(PIN_BOTAO_PARADA) == LOW) {
        delay(10);
      }
    }
  }

  unsigned long tempoAtual = millis();

  // Lógica de Leitura do Joystick, Pisca do LED RGB e Monitoramento Contínuo
  if (sistemaLigado) {
    // 1. Leitura automática dos sensores a cada 2 segundos (para alertas de emergência)
    if (tempoAtual - tempoAnteriorSensores >= intervaloSensores) {
      tempoAnteriorSensores = tempoAtual;
      verificarSensores();
    }

    // 2. Leitura do Joystick
    int valorX = analogRead(PIN_VRX);
    int valorY = analogRead(PIN_VRY);

    String comandoAtual = "Parado";

    if (valorY < 1000) {
      comandoAtual = "Frente -> motores para frente.";
    } else if (valorY > 3000) {
      comandoAtual = "Trás -> motores para trás.";
    } else if (valorX < 1000) {
      comandoAtual = "Esquerda -> motor direito ativo.";
    } else if (valorX > 3000) {
      comandoAtual = "Direita -> motor esquerdo ativo.";
    }

    if (comandoAtual != ultimoComando) {
      Serial.print("Comando enviado: ");
      Serial.println(comandoAtual);
      ultimoComando = comandoAtual;
    }

    // 3. Controle de tempo para alternar o pisca (não-bloqueante)
    if (tempoAtual - tempoAnteriorRGB >= intervaloPisca) {
      tempoAnteriorRGB = tempoAtual;
      estadoRGB = !estadoRGB;
    }

    // Cor baseada no mesmo comando mostrado no Serial
    if (estadoRGB) {
      if (comandoAtual.startsWith("Frente"))        setCorRGB(false, false, true);  // Azul
      else if (comandoAtual.startsWith("Esquerda")) setCorRGB(true,  true,  false); // Amarelo
      else if (comandoAtual.startsWith("Direita"))  setCorRGB(true,  false, true);  // Magenta
      else                                          setCorRGB(false, false, false); // Parado/Trás
    } else {
      setCorRGB(false, false, false);
    }
  } else {
    setCorRGB(false, false, false);
  }

  delay(20);
}
