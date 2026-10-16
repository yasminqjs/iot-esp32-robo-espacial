# iot-esp32-robo-espacial

## Nome do projeto

**Robô Espacial IoT com ESP32 — Controle Remoto**

Projeto em que um robô calcula a probabilidade de vida com base em sensores e em um algoritmo de aprendizado de máquina, sendo controlado remotamente por um controle baseado em ESP32.

## Objetivo da etapa

Desenvolver e simular o **controle remoto do robô** usando a placa **ESP32 DevKit v1**, com:

- leitura de um joystick analógico para gerar os comandos de movimento (frente, trás, esquerda, direita ou parado);
- LEDs de status (verde = robô conectado/ativo, vermelho = robô desligado);
- um botão para parada/religamento remoto do sistema;
- envio dos comandos e do status pelo Monitor Serial (115200 baud).

## Lista de componentes do circuito

| Componente | Quantidade | Pino do ESP32 | Função |
|---|---|---|---|
| ESP32 DevKit v1 | 1 | — | Microcontrolador do controle remoto |
| Joystick analógico | 1 | VRX → GPIO 34, VRY → GPIO 35 | Leitura dos eixos X e Y (direção do robô) |
| LED verde | 1 | GPIO 18 | Status: robô conectado e ativo |
| LED vermelho | 1 | GPIO 19 | Status: robô desligado |
| Botão / interruptor | 1 | GPIO 23 (INPUT_PULLUP) | Parada e religamento do sistema |
| LED RGB | 1 | — | Previsto no projeto; não utilizado no código desta etapa |

## Link do projeto

https://velxio.dev/victor-goncalves/robo/

## Como rodar no Wokwi

1. Abra o link do projeto acima no navegador.
2. Confira se o circuito está montado com a placa ESP32 DevKit v1, o joystick, os LEDs verde e vermelho e o botão, conforme a tabela de pinos.
3. Verifique se o código do arquivo `controle_remoto_robo.ino` está carregado no editor.
4. Clique em **Start the simulation** (botão verde de play).
5. Abra o **Monitor Serial** e confira a mensagem de inicialização, com o LED verde aceso.
6. Mova o joystick nas quatro direções e acompanhe os comandos impressos no monitor.
7. Pressione o botão para desligar (LED vermelho aceso) e pressione novamente para religar (LED verde aceso).
