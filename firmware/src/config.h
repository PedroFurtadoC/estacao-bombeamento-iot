#pragma once

// ============================================================
// Configuracao geral do no de monitoramento
// Limiares e faixas: docs/03-modelo-de-dados.md
// ============================================================

// Intervalo entre publicacoes de telemetria (ms)
#define INTERVALO_PUBLICACAO_MS 10000UL

// Fuso horario (Brasil, UTC-3) para o timestamp via NTP
#define FUSO_SEGUNDOS (-3 * 3600)
#define SERVIDOR_NTP_1 "pool.ntp.org"
#define SERVIDOR_NTP_2 "time.google.com"

// ---------------- Pinos ----------------
#define PINO_DHT 4        // DHT22/AM2302 (todas as maquinas)
#define PINO_ANALOGICO 34 // HW-484 A0 (M02) ou MQ A0 (M03) - ADC1
#define PINO_VAZAO 33     // sensor de vazao hall, pulsos (M01 e M03)
#define PINO_LED_R 25     // LED RGB HW-479 (M01)
#define PINO_LED_G 26
#define PINO_LED_B 27
#define PINO_LED_FLASH 25 // LED flash HW-481 (M03)

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

// Conversao pulsos -> L/min. YF-S201: 7.5 Hz por L/min (ajustar por modelo).
#ifndef FATOR_VAZAO_HZ_POR_LMIN
#define FATOR_VAZAO_HZ_POR_LMIN 7.5f
#endif

// Em bancada sem agua a vazao real e 0 (leitura correta: bomba "a seco"),
// o que deixaria a maquina em alerta critico permanente. Por padrao a vazao
// e publicada mas NAO entra no calculo do status; ligue com
// -D VAZAO_AFETA_STATUS=1 quando houver agua circulando na demonstracao.
#ifndef VAZAO_AFETA_STATUS
#define VAZAO_AFETA_STATUS 0
#endif

// ---------------- Simulacao ----------------
// Sinais sem sensor fisico sao simulados com variacao realista.
// Uma janela de anomalia e injetada periodicamente (Desafio 2).
#define SIM_ANOMALIA_PERIODO_MS (15UL * 60UL * 1000UL) // a cada 15 min
#define SIM_ANOMALIA_DURACAO_MS (60UL * 1000UL)        // dura 60 s
