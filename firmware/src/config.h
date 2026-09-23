#pragma once

// Configuracao do no de monitoramento.
// Faixas e limiares: docs/03-modelo-de-dados.md
// Ligacao pino a pino: docs/10-esquema-eletrico.md

#define INTERVALO_PUBLICACAO_MS 10000UL

#define FUSO_SEGUNDOS (-3 * 3600) // UTC-3
#define SERVIDOR_NTP_1 "pool.ntp.org"
#define SERVIDOR_NTP_2 "time.google.com"

// ---------------- Pinos ----------------
// O mapa e escolhido pelo alvo compilado, que vem do "board" do environment.
#if CONFIG_IDF_TARGET_ESP32S2
// LOLIN S2 Mini (sem uso nesta montagem). So a fileira externa tem header:
//   esquerda: EN 3 5 7 9 11 12 3V3  |  direita: 39 37 35 33 18 16 GND VBUS
#define PINO_DHT 7
#define PINO_VAZAO 5
#define PINO_MQ_AO 3        // ADC1 (o ADC2 para com o Wi-Fi ligado)
#define PINO_LED_R 9
#define PINO_LED_G 11
#define PINO_LED_B 12
#define PINO_LED_FLASH 9
#define PINO_LED_ONBOARD 15

#elif CONFIG_IDF_TARGET_ESP32S3
// ESP32-S3-N16R8 (M02). Nao usar: 0/3/45/46 (boot), 19/20 (USB),
// 35/36/37 (PSRAM octal), 43/44 (serial), 48 (LED da placa).
#define PINO_DHT 4
#define PINO_VAZAO 5
#define PINO_MQ_AO 6        // ADC1 (o ADC2 para com o Wi-Fi ligado)
#define PINO_LED_R 15
#define PINO_LED_G 16
#define PINO_LED_B 17
#define PINO_LED_FLASH 15
#define PINO_LED_ONBOARD RGB_BUILTIN // LED enderecavel; o core aceita digitalWrite

#else
// ESP32 DevKit V1 / WROOM-32 (M01 e M03).
// Nao usar: 0/2/12/15 (boot). 34/35/36/39 sao somente entrada.
#define PINO_DHT 4
#define PINO_MQ_AO 34       // ADC1
#define PINO_VAZAO 33
#define PINO_LED_R 25
#define PINO_LED_G 26
#define PINO_LED_B 27
#define PINO_LED_FLASH 25
#define PINO_LED_ONBOARD 2
#endif

// 0: sensor de vazao em 5 V, com divisor 10k/20k (pino em INPUT).
// 1: sensor em 3,3 V, sinal direto no GPIO e pull-up do proprio ESP32.
#ifndef VAZAO_PULLUP_INTERNO
#define VAZAO_PULLUP_INTERNO 0
#endif

// Leva a leitura ambiente do DHT22 para a faixa do cenario (65-75 C).
// 40 e calibrado para sala de ~30 C; com 45 a leitura dava 75,1 C e a maquina
// ja nascia em atencao. Use -D TEMP_OFFSET=0.0f para leitura crua.
#ifndef TEMP_OFFSET
#define TEMP_OFFSET 40.0f
#endif

// ---------------- Limiares de status (0 normal, 1 atencao, 2 critico) ------
#define TEMP_ATENCAO 75.0f
#define TEMP_CRITICO 80.0f

#define VIB_ATENCAO 3.5f
#define VIB_CRITICO 5.0f

#define CORR_ATENCAO 9.0f
#define CORR_CRITICO 11.0f

// Motor de 2 polos em 60 Hz: nominal ~3500 RPM
#define ROT_CRITICO_MIN 3300.0f
#define ROT_ATENCAO_MIN 3400.0f
#define ROT_ATENCAO_MAX 3600.0f
#define ROT_CRITICO_MAX 3700.0f

// Gas em % do fundo de escala do ADC, nao em ppm (o MQ nao foi calibrado com
// gas de referencia). Em ar limpo e aquecido o sensor da 31 %, por isso 45/60.
#define GAS_ATENCAO 45.0f
#define GAS_CRITICO 60.0f

#define VAZAO_CRITICO_MIN 10.0f
#define VAZAO_ATENCAO_MIN 20.0f
#define VAZAO_ATENCAO_MAX 40.0f
#define VAZAO_CRITICO_MAX 45.0f

// f(Hz) = FATOR * Q(L/min). Por maquina no platformio.ini.
#ifndef FATOR_VAZAO_HZ_POR_LMIN
#define FATOR_VAZAO_HZ_POR_LMIN 7.5f
#endif

// Bancada seca le 0 L/min, o que alarmaria sem parar. Por isso a vazao e
// publicada mas fica fora do status ate alguem ligar VAZAO_AFETA_STATUS=1.
// Cuidado: o YF-S402 (M02) so vai ate 6 L/min, abaixo da faixa normal de
// 20-40 - com a flag ligada ele alarma mesmo com agua.
#ifndef VAZAO_AFETA_STATUS
#define VAZAO_AFETA_STATUS 0
#endif

// ---------------- Simulacao (Desafio 2) ----------------
#define SIM_ANOMALIA_PERIODO_MS (15UL * 60UL * 1000UL)
#define SIM_ANOMALIA_DURACAO_MS (60UL * 1000UL)
