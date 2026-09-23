#pragma once

#include <Arduino.h>
#include "config.h"

// Leitura dos sensores fisicos. Cada bloco e independente e so entra no build
// se o environment definir a flag. Alem do valor processado, cada funcao
// devolve o bruto (contagens do ADC, pulsos) para conferir na bancada.

#if defined(SENSOR_DHT22)
#include <DHT.h>
static DHT dht(PINO_DHT, DHT22);

// false = sensor nao respondeu (NaN): fio solto, pino errado ou sem pull-up.
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
// MQ-2/MQ-135 em % do fundo de escala do ADC. O elemento precisa de ~2 min
// de aquecimento antes de dar leitura confiavel.
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
// Turbina hall: a frequencia dos pulsos e proporcional a vazao. Contamos por
// interrupcao e convertemos na janela de publicacao.
static volatile uint32_t pulsosVazao = 0;
static portMUX_TYPE muxVazao = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR aoDetectarPulsoVazao() {
    portENTER_CRITICAL_ISR(&muxVazao);
    pulsosVazao++;
    portEXIT_CRITICAL_ISR(&muxVazao);
}

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
    // Fixa 12 bits (o S2 nasce em 13) para a escala de gas nao mudar com a placa
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db); // faixa util ate ~3,1 V
    pinMode(PINO_MQ_AO, INPUT);
#endif
#if defined(SENSOR_VAZAO)
    pinMode(PINO_VAZAO, VAZAO_PULLUP_INTERNO ? INPUT_PULLUP : INPUT);
    attachInterrupt(digitalPinToInterrupt(PINO_VAZAO), aoDetectarPulsoVazao, FALLING);
#endif
}
