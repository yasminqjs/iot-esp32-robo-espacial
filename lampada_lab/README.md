# Monitoramento e Controle com ESP32-S3

Projeto com ESP32-S3 que lê temperatura, luminosidade e um potenciômetro, e controla um LED RGB e um buzzer. As leituras são enviadas para a nuvem, armazenadas em um banco de dados e exibidas em um dashboard no Grafana.

## Conteúdo do repositório

- `esp32.ino`: código do ESP32-S3.
- `README.md`: documentação do projeto.

## Como montar o projeto

### Componentes

- ESP32-S3
- Sensor de temperatura
- Fotorresistor (LDR) com resistor divisor de tensão
- Potenciômetro
- Botão
- Buzzer
- LED RGB (cátodo comum) com 3 resistores

### Ligações

| Componente            | Pino    |
|-----------------------|---------|
| Sensor de temperatura | GPIO 4  |
| LDR                   | GPIO 5  |
| Potenciômetro         | GPIO 6  |
| Botão (outro lado no GND) | GPIO 7 |
| Buzzer                | GPIO 15 |
| LED RGB – Vermelho    | GPIO 9  |
| LED RGB – Verde       | GPIO 10 |
| LED RGB – Azul        | GPIO 11 |

### Gravação do firmware

1. Instale a Arduino IDE e o pacote de placas ESP32 (versão 2.x).
2. Instale a biblioteca ArduinoIoTCloud.
3. Abra o `esp32.ino`, selecione a placa ESP32-S3 e faça o upload.
4. Abra o Monitor Serial em 115200 baud para acompanhar o funcionamento.

### Funcionamento

- O botão liga e desliga o sistema.
- O buzzer toca quando a temperatura fica abaixo de 0 °C ou acima de 25 °C.
- O LED RGB acende quando está escuro (LDR) ou quando recebe o comando `ligar`, e a cor é escolhida pelo potenciômetro.
- Comandos aceitos: `ligar`, `desligar`, `vermelho`, `amarelo`, `ativar/desativar temperatura`, `ativar/desativar detector` e `ativar/desativar buzzer`.

## Como rodar o backend

O backend recebe os dados enviados pelo ESP32 (temperatura, luminosidade, alerta e estado do LED) e os grava no banco de dados. Para rodá-lo:

1. Configure as credenciais de acesso à nuvem e ao banco de dados.
2. Instale as dependências do backend.
3. Inicie o backend e confirme que as leituras do ESP32 estão chegando.

## Descrição do Banco de Dados

O banco de dados guarda o histórico das leituras do ESP32-S3. Cada registro representa uma leitura e contém:

| Campo         | Descrição                                   |
|---------------|---------------------------------------------|
| `timestamp`   | Data e hora da leitura                      |
| `temperatura` | Temperatura em °C                           |
| `luminosidade`| Valor bruto do LDR (0 a 4095)               |
| `alerta`      | Indica se a temperatura saiu da faixa de 0 a 25 °C |
| `led_ligado`  | Estado do LED RGB                           |

## Descrição do Grafana

O Grafana é conectado ao banco de dados e apresenta as leituras em um dashboard, com painéis de:

- Temperatura ao longo do tempo e valor atual
- Luminosidade ao longo do tempo
- Alertas de temperatura
- Estado do LED
