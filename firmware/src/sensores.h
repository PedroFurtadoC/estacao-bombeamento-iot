#pragma once

#include <Arduino.h>
#include "config.h"

// ============================================================
// Leitura dos sensores fisicos. Os blocos sao independentes:
// uma maquina pode combinar DHT22 + vazao ou DHT22 + MQ (build flags).
// Os pinos vem de config.h e mudam conforme a placa (S2 Mini / DevKit).
// Cada funcao devolve o valor processado e, opcionalmente, o valor bruto
// (contagens do ADC, pulsos) para conferencia no monitor serial.
// ============================================================

#if defined(SENSOR_DHT22)
#include <DHT.h>
static DHT dht(PINO_DHT, DHT22);

// Le temperatura (sem offset) e umidade de uma vez. Retorna false se o sensor
// nao respondeu (NaN): fio de dados solto, pino errado ou falta de pull-up.
// A temperatura "industrial" do cenario e tempBruta + TEMP_OFFSET (config.h).
inline bool lerDHT(float &tempBruta, float &umidade) {
    float t = dht.readTemperature();
    float u = dht.readHumidity();
    if (isnan(t) || isnan(u)) return false;
    tempBruta = t;
    umidade = u;
    return true;
}
#endif

#if defined(SENSOR_MQ)
// MQ-2/MQ-135: gas/qualidade do ar da casa de bombas (espaco confinado),
// em % do fundo de escala do ADC (A0 chega via divisor 10k/20k).
// adc (opcional) recebe a media bruta em contagens (0..4095).
// Obs.: o elemento sensor precisa de ~2 min de aquecimento apos ligar.
inline float lerGas(uint16_t *adc = nullptr) {
    uint32_t soma = 0;
    for (int i = 0; i < 16; i++) {
        soma += analogRead(PINO_MQ_AO);
        delay(2);
    }
    float media = soma / 16.0f;
    if (adc) *adc = (uint16_t)media;
    return (media / 4095.0f) * 100.0f;
}
#endif

#if defined(SENSOR_VAZAO)
// Sensor de vazao hall (YF-S201C na M01, YF-S402 na M02): a turbina gera
// pulsos com frequencia proporcional a vazao. Contamos os pulsos por
// interrupcao e convertemos em L/min na janela de publicacao, com o fator
// do modelo (FATOR_VAZAO_HZ_POR_LMIN, definido por environment).
// Ligacao: 5 V + divisor/pull-up (docs/10) ou 3,3 V + VAZAO_PULLUP_INTERNO=1.
static volatile uint32_t pulsosVazao = 0;
static portMUX_TYPE muxVazao = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR aoDetectarPulsoVazao() {
    portENTER_CRITICAL_ISR(&muxVazao);
    pulsosVazao++;
    portEXIT_CRITICAL_ISR(&muxVazao);
}

// pulsosLidos (opcional) recebe a contagem bruta da janela.
inline float lerVazao(unsigned long janelaMs, uint32_t *pulsosLidos = nullptr) {
    portENTER_CRITICAL(&muxVazao);
    uint32_t pulsos = pulsosVazao;
    pulsosVazao = 0;
    portEXIT_CRITICAL(&muxVazao);
    if (pulsosLidos) *pulsosLidos = pulsos;
    if (janelaMs == 0) return 0.0f;
    float hz = (pulsos * 1000.0f) / (float)janelaMs;
    return hz / FATOR_VAZAO_HZ_POR_LMIN;
}
#endif

inline void iniciarSensores() {
#if defined(SENSOR_DHT22)
    dht.begin();
#endif
#if defined(SENSOR_MQ)
    // 12 bits nas duas familias (o ESP32-S2 nasce em 13 bits), assim a
    // escala de gas fica igual em qualquer placa.
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db); // faixa util ate ~3,1 V
#endif
#if defined(SENSOR_MQ)
    pinMode(PINO_MQ_AO, INPUT);
#endif
#if defined(SENSOR_VAZAO)
    pinMode(PINO_VAZAO, VAZAO_PULLUP_INTERNO ? INPUT_PULLUP : INPUT);
    attachInterrupt(digitalPinToInterrupt(PINO_VAZAO), aoDetectarPulsoVazao, FALLING);
#endif
}
