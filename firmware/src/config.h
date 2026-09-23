#pragma once

// ============================================================
// Configuracao geral do no de monitoramento
// Limiares e faixas: docs/03-modelo-de-dados.md
// Ligacao pino a pino: docs/10-esquema-eletrico.md
// ============================================================

// Intervalo entre publicacoes de telemetria (ms)
#define INTERVALO_PUBLICACAO_MS 10000UL

// Fuso horario (Brasil, UTC-3) para o timestamp via NTP
#define FUSO_SEGUNDOS (-3 * 3600)
#define SERVIDOR_NTP_1 "pool.ntp.org"
#define SERVIDOR_NTP_2 "time.google.com"

// ---------------- Pinos ----------------
// A placa vem do "board" do environment (platformio.ini); o alvo compilado
// (ESP32 classico, ESP32-S2 ou ESP32-S3) escolhe o mapa de pinos.
#if CONFIG_IDF_TARGET_ESP32S2
// LOLIN S2 Mini (opcional, nao usado nesta montagem). So a fileira EXTERNA tem header:
//   esquerda: EN  3  5  7  9  11  12  3V3
//   direita : 39  37  35  33  18  16  GND  VBUS
// ADC1 do S2 = GPIO 1..10 (o ADC2 para com o Wi-Fi ligado).
#define PINO_DHT 7          // DHT22/AM2302 DATA
#define PINO_VAZAO 5        // sensor de vazao hall, pulsos
#define PINO_MQ_AO 3        // MQ A0 (se algum dia for usado no S2) - ADC1_CH2
#define PINO_LED_R 9        // LED RGB HW-479 (M01)
#define PINO_LED_G 11
#define PINO_LED_B 12
#define PINO_LED_FLASH 9    // LED flash HW-481
#define PINO_LED_ONBOARD 15 // LED azul da placa (fileira interna, sem header)
#elif CONFIG_IDF_TARGET_ESP32S3
// ESP32-S3-N16R8, formato DevKitC-1 de 44 pinos (M02: DHT22 + vazao).
// Pinos evitados: 0, 3, 45, 46 (boot); 19, 20 (USB); 35, 36, 37 (PSRAM octal
// do R8); 43, 44 (UART0 = serial do monitor); 48 (LED RGB da placa).
// ADC1 do S3 = GPIO 1..10 (o ADC2 para com o Wi-Fi ligado).
#define PINO_DHT 4          // DHT22/AM2302 DATA (mesmo numero da DevKit)
#define PINO_VAZAO 5        // sensor de vazao hall, pulsos
#define PINO_MQ_AO 6        // MQ A0 via divisor - ADC1_CH5
#define PINO_LED_R 15       // LED RGB HW-479
#define PINO_LED_G 16
#define PINO_LED_B 17
#define PINO_LED_FLASH 15   // LED flash HW-481
#define PINO_LED_ONBOARD RGB_BUILTIN // LED RGB enderecavel da placa (GPIO 48);
                                     // o core trata digitalWrite nele: acende branco
#else
// ESP32 DevKit V1 / WROOM-32 (M01: DHT22 + vazao + LED RGB; M03: DHT22 + MQ + LED flash)
// Pinos evitados: 0, 2, 12, 15 (boot); 34/35/36/39 sao somente entrada.
#define PINO_DHT 4          // DHT22/AM2302 DATA
#define PINO_MQ_AO 34       // MQ A0 via divisor - ADC1, somente entrada (M03)
#define PINO_VAZAO 33       // sensor de vazao hall, pulsos via divisor (M01)
#define PINO_LED_R 25       // LED RGB HW-479 (M01)
#define PINO_LED_G 26
#define PINO_LED_B 27
#define PINO_LED_FLASH 25   // LED flash HW-481 (M03)
#define PINO_LED_ONBOARD 2  // LED azul da DevKit
#endif

// Pull-up interno no pino de vazao.
//   0 (padrao): sensor em 5 V com divisor 10k/20k e/ou pull-up externo,
//               como em docs/10-esquema-eletrico.md; o pino fica em INPUT.
//   1         : sensor alimentado em 3,3 V e sinal direto no GPIO, sem
//               divisor; o proprio ESP32 fornece o pull-up (INPUT_PULLUP).
#ifndef VAZAO_PULLUP_INTERNO
#define VAZAO_PULLUP_INTERNO 0
#endif

// Offset didatico: leva a leitura ambiente do DHT22 (~25 C) para a faixa
// industrial do cenario (65-75 C). Aquecer o sensor com o dedo dispara a
// anomalia ao vivo. Use -D TEMP_OFFSET=0.0f para leitura crua.
#ifndef TEMP_OFFSET
#define TEMP_OFFSET 45.0f
#endif

// ---------------- Limiares de status ----------------
// status: 0 = normal | 1 = atencao | 2 = critico
#define TEMP_ATENCAO 75.0f
#define TEMP_CRITICO 80.0f

#define VIB_ATENCAO 3.5f
#define VIB_CRITICO 5.0f

#define CORR_ATENCAO 9.0f
#define CORR_CRITICO 11.0f

// Conjunto motobomba com motor de 2 polos (60 Hz): nominal ~3500 RPM
#define ROT_CRITICO_MIN 3300.0f
#define ROT_ATENCAO_MIN 3400.0f
#define ROT_ATENCAO_MAX 3600.0f
#define ROT_CRITICO_MAX 3700.0f

#define GAS_ATENCAO 20.0f
#define GAS_CRITICO 40.0f

// Vazao da motobomba (L/min): queda indica obstrucao/cavitacao; zero, bomba a seco
#define VAZAO_CRITICO_MIN 10.0f
#define VAZAO_ATENCAO_MIN 20.0f
#define VAZAO_ATENCAO_MAX 40.0f
#define VAZAO_CRITICO_MAX 45.0f

// Conversao pulsos -> L/min: f(Hz) = FATOR * Q(L/min). Definido por maquina
// no platformio.ini: YF-S201C = 7,5 (M01); YF-S402 = 73 (M02); YF-S201 = 7,5.
#ifndef FATOR_VAZAO_HZ_POR_LMIN
#define FATOR_VAZAO_HZ_POR_LMIN 7.5f
#endif

// Em bancada sem agua a vazao real e 0 (leitura correta: bomba "a seco"),
// o que deixaria a maquina em alerta critico permanente. Por padrao a vazao
// e publicada mas NAO entra no calculo do status; ligue com
// -D VAZAO_AFETA_STATUS=1 quando houver agua circulando na demonstracao.
// Obs.: o YF-S402 (M02) mede no maximo 6 L/min, abaixo da faixa "normal"
// de 20-40 L/min do cenario - com VAZAO_AFETA_STATUS=1 a M02 ficaria em
// atencao/critico mesmo com agua; ajuste os limiares acima se for o caso.
#ifndef VAZAO_AFETA_STATUS
#define VAZAO_AFETA_STATUS 0
#endif

// ---------------- Simulacao ----------------
// Sinais sem sensor fisico sao simulados com variacao realista.
// Uma janela de anomalia e injetada periodicamente (Desafio 2).
#define SIM_ANOMALIA_PERIODO_MS (15UL * 60UL * 1000UL) // a cada 15 min
#define SIM_ANOMALIA_DURACAO_MS (60UL * 1000UL)        // dura 60 s
