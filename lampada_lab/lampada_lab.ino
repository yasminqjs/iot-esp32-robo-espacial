#include "thingProperties.h"

// ============================================================================
// MAPEAMENTO DE PINOS (ESP32-S3 - Todos os sensores analogicos no ADC1)
// ============================================================================
#define PIN_TEMP        4   // Sensor de Temperatura
#define PIN_LDR         5   // Fotorresistor (LDR)
#define PIN_POT         6   // Potenciometro
#define PIN_BOTAO       7   // Botao (Com interrupcao externa para GND)
#define PIN_BUZZER      15  // Buzzer
#define PIN_LED_R       9   // LED RGB - Vermelho
#define PIN_LED_G       10  // LED RGB - Verde
#define PIN_LED_B       11  // LED RGB - Azul

// Canais PWM (LEDC) do LED RGB - core ESP32 2.x
// Usa canais 3, 4 e 5 para nao conflitar com o tone() do buzzer
#define CH_R            3
#define CH_G            4
#define CH_B            5

// ============================================================================
// CONFIGURACOES DE HARDWARE E CALIBRACAO
// ============================================================================
#define TIPO_SENSOR_TEMP  1     // 0 = LM35 | 1 = TMP36 | 2 = NTC 10k
#define LIMITE_LUZ        1500  // Limite LDR (Valores menores = escuro)
#define LED_ANODO_COMUM   0     // 0 = Catodo Comum | 1 = Anodo Comum

// ============================================================================
// VARIAVEIS INTERNAS DE CONTROLE DO SISTEMA
// ============================================================================
bool sistemaAtivo   = true;
bool tempAtiva      = true;
bool detectorAtivo  = true;
bool buzzerAtivo    = true;
bool vontadeLigar   = false;

// Controle de Interrupcao do Botao
volatile bool botaoPressionado = false;
unsigned long ultimoDebounce = 0;

// Controle de Cor Temporizada (1 segundo)
bool corForcadaAtiva = false;
unsigned long tempoCorForcada = 0;
int rForcado = 0, gForcado = 0, bForcado = 0;

// Temporizador do Monitor Serial
unsigned long ultimoPrintSerial = 0;

// Prototipos
void setCorTemporaria(int r, int g, int b);
float lerTemperatura();
void calcularEspectroRGB(int pot, int &r, int &g, int &b);
void aplicarRGB(int r, int g, int b);

// Interrupcao do Botao (IRAM_ATTR para execucao rapida na RAM)
void IRAM_ATTR ISR_Botao() {
  botaoPressionado = true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  // Configuracao dos Pinos
  pinMode(PIN_BOTAO, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // Interrupcao externa no botao (borda de descida / FALLING)
  attachInterrupt(digitalPinToInterrupt(PIN_BOTAO), ISR_Botao, FALLING);

  // Configuracao do PWM do LED RGB (Core ESP32 2.x)
  ledcSetup(CH_R, 5000, 8);
  ledcAttachPin(PIN_LED_R, CH_R);
  ledcSetup(CH_G, 5000, 8);
  ledcAttachPin(PIN_LED_G, CH_G);
  ledcSetup(CH_B, 5000, 8);
  ledcAttachPin(PIN_LED_B, CH_B);

  // Inicializacao do Arduino IoT Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

void loop() {
  ArduinoCloud.update();

  // 1. Trata a Interrupcao do Botao Fisico
  if (botaoPressionado) {
    botaoPressionado = false;
    if (millis() - ultimoDebounce > 250) {
      sistemaAtivo = !sistemaAtivo;
      ultimoDebounce = millis();
      Serial.print("-> Botao Pressionado! Sistema: ");
      Serial.println(sistemaAtivo ? "ATIVADO" : "DESATIVADO");
    }
  }

  // 2. Leitura dos Sensores
  temperatura = lerTemperatura();
  luminosidade = analogRead(PIN_LDR);
  int valPot = analogRead(PIN_POT);

  // 3. Verificacao de Alerta de Temperatura (< 0 ou > 25 graus)
  bool tempForaDoRango = (temperatura < 0.0 || temperatura > 25.0);
  alerta = tempForaDoRango;

  // 4. Logica do Buzzer
  if (sistemaAtivo && buzzerAtivo && tempForaDoRango) {
    tone(PIN_BUZZER, 1000);
  } else {
    noTone(PIN_BUZZER);
    digitalWrite(PIN_BUZZER, LOW);
  }

  // 5. Logica de Acendimento do LED RGB
  bool escuro = (luminosidade < LIMITE_LUZ);
  bool sensorPermiteLigar = detectorAtivo ? escuro : true;
  bool tempPermiteLigar   = tempAtiva ? !tempForaDoRango : true;

  if (sistemaAtivo && tempPermiteLigar && (vontadeLigar || sensorPermiteLigar)) {
    ledLigado = true;
  } else {
    ledLigado = false;
  }

  // 6. Atualizacao de Cor do LED RGB
  if (ledLigado) {
    if (corForcadaAtiva && (millis() - tempoCorForcada >= 1000)) {
      corForcadaAtiva = false;
    }
    if (corForcadaAtiva) {
      aplicarRGB(rForcado, gForcado, bForcado);
    } else {
      int r, g, b;
      calcularEspectroRGB(valPot, r, g, b);
      aplicarRGB(r, g, b);
    }
  } else {
    aplicarRGB(0, 0, 0);
  }

  // 7. Exibicao Periodica no Monitor Serial
  if (millis() - ultimoPrintSerial >= 1000) {
    ultimoPrintSerial = millis();
    Serial.print("LED: ");
    Serial.print(ledLigado ? "ON" : "OFF");
    Serial.print(" | Temp: ");
    Serial.print(temperatura, 1);
    Serial.print(" C | Luz: ");
    Serial.print(luminosidade);
    if (tempForaDoRango) Serial.print(" | *** PERIGO! DESLIGAR! ***");
    Serial.println();
  }
}

// ============================================================================
// FUNCOES AUXILIARES
// ============================================================================

void onComandoChange() {
  String cmd = comando;
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "ligar") {
    vontadeLigar = true;
  } else if (cmd == "desligar") {
    vontadeLigar = false;
  } else if (cmd == "vermelho") {
    setCorTemporaria(255, 0, 0);
  } else if (cmd == "amarelo") {
    setCorTemporaria(255, 255, 0);
  } else if (cmd == "ativar temperatura") {
    tempAtiva = true;
  } else if (cmd == "desativar temperatura") {
    tempAtiva = false;
  } else if (cmd == "ativar detector") {
    detectorAtivo = true;
  } else if (cmd == "desativar detector") {
    detectorAtivo = false;
  } else if (cmd == "ativar buzzer") {
    buzzerAtivo = true;
  } else if (cmd == "desativar buzzer") {
    buzzerAtivo = false;
  }
}

void setCorTemporaria(int r, int g, int b) {
  rForcado = r;
  gForcado = g;
  bForcado = b;
  corForcadaAtiva = true;
  tempoCorForcada = millis();
}

float lerTemperatura() {
  int raw = analogRead(PIN_TEMP);
  float tensao = (raw * 3.3) / 4095.0;

  #if TIPO_SENSOR_TEMP == 0
    return tensao * 100.0;             // LM35 (10mV/C)
  #elif TIPO_SENSOR_TEMP == 1
    return (tensao - 0.5) * 100.0;     // TMP36
  #else
    // Aproximacao linear simplificada para NTC 10k
    return (3.3 - tensao) * 20.0;
  #endif
}

// Espectro de Cor: Vermelho -> Laranja -> Amarelo -> Branco -> Azul Turquesa -> Azul
void calcularEspectroRGB(int pot, int &r, int &g, int &b) {
  if (pot < 819) {             // Vermelho ate Laranja
    float t = pot / 819.0;
    r = 255; g = 127 * t; b = 0;
  } else if (pot < 1638) {     // Laranja ate Amarelo
    float t = (pot - 819) / 819.0;
    r = 255; g = 127 + (128 * t); b = 0;
  } else if (pot < 2457) {     // Amarelo ate Branco
    float t = (pot - 1638) / 819.0;
    r = 255; g = 255; b = 255 * t;
  } else if (pot < 3276) {     // Branco ate Azul Turquesa
    float t = (pot - 2457) / 819.0;
    r = 255 * (1.0 - t); g = 255; b = 255;
  } else {                     // Turquesa ate Azul Puro
    float t = (pot - 3276) / 819.0;
    if (t > 1.0) t = 1.0;
    r = 0; g = 255 * (1.0 - t); b = 255;
  }
}

void aplicarRGB(int r, int g, int b) {
  #if LED_ANODO_COMUM == 1
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  #endif
  ledcWrite(CH_R, r);
  ledcWrite(CH_G, g);
  ledcWrite(CH_B, b);
}
