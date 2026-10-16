/*
  Firmware: Controle Remoto do Robô com ESP32
  Placa: ESP32 DevKit v1 (Simulador Wokwi)
*/

// Mapeamento dos Pinos
const int PIN_VRX = 34;          // Leitura analógica Eixo X
const int PIN_VRY = 35;          // Leitura analógica Eixo Y
const int PIN_LED_VERDE = 18;     // Status: Conectado e Ativo
const int PIN_LED_VERMELHO = 19;  // Status: Desligado
const int PIN_BOTAO_PARADA = 23;  // Botão de parada remota

// Variáveis de Controle
bool sistemaLigado = true;
String ultimoComando = "";

void setup() {
  Serial.begin(115200);

  // Configuração dos pinos
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BOTAO_PARADA, INPUT_PULLUP);

  // Estado Inicial: Robô ligado e pronto
  digitalWrite(PIN_LED_VERDE, HIGH);
  digitalWrite(PIN_LED_VERMELHO, LOW);

  Serial.println("==========================================");
  Serial.println("  CONTROLE REMOTO INICIALIZADO COM SUCESSO");
  Serial.println("  Status: Robô Conectado (LED Verde Aceso)");
  Serial.println("==========================================");
}

void loop() {
  // Leitura do Botão de Desligamento Remoto
  if (digitalRead(PIN_BOTAO_PARADA) == LOW) {
    delay(50); // Debounce simples
    if (digitalRead(PIN_BOTAO_PARADA) == LOW) {
      if (sistemaLigado) {
        sistemaLigado = false;
        
        // Atualiza LEDs de status do controle
        digitalWrite(PIN_LED_VERDE, LOW);
        digitalWrite(PIN_LED_VERMELHO, HIGH);

        // Mensagens no Monitor Serial
        Serial.println("\n[ALERTA] Comando enviado: DESLIGAR");
        Serial.println("Status: Robô Desligado / Motores Parados (LED Vermelho Aceso)\n");
      } else {
        // Pressionar novamente religa o sistema para testes
        sistemaLigado = true;
        digitalWrite(PIN_LED_VERDE, HIGH);
        digitalWrite(PIN_LED_VERMELHO, LOW);
        
        Serial.println("\n[INFO] Comando enviado: LIGAR");
        Serial.println("Status: Robô Conectado (LED Verde Aceso)\n");
      }

      // Aguarda a liberação do botão para evitar repetição contínua
      while (digitalRead(PIN_BOTAO_PARADA) == LOW) {
        delay(10);
      }
    }
  }

  // Lógica de Leitura do Joystick (Executada apenas com o sistema LIGADO)
  if (sistemaLigado) {
    int valorX = analogRead(PIN_VRX);
    int valorY = analogRead(PIN_VRY);

    String comandoAtual = "Parado";

    // Leitura das 4 direções (Valores do ADC do ESP32: 0 a 4095; Centro ~2048)
    if (valorY < 1000) {
      comandoAtual = "Frente -> motores para frente.";
    } else if (valorY > 3000) {
      comandoAtual = "Trás -> motores para trás.";
    } else if (valorX < 1000) {
      comandoAtual = "Esquerda -> motor direito ativo.";
    } else if (valorX > 3000) {
      comandoAtual = "Direita -> motor esquerdo ativo.";
    }

    // Atualiza o monitor serial apenas quando houver mudança de direção
    if (comandoAtual != ultimoComando) {
      Serial.print("Comando enviado: ");
      Serial.println(comandoAtual);
      ultimoComando = comandoAtual;
    }
  }

  delay(100);
}
