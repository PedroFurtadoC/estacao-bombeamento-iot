#pragma once

#include <Arduino.h>
#include "config.h"

// ============================================================
// Leitura dos sensores fisicos. Os blocos sao independentes:
// uma maquina pode combinar DHT22 + sensor analogico (build flags).
// ============================================================

#if defined(SENSOR_DHT22)
#include <DHT.h>
static DHT dht(PINO_DHT, DHT22);

// Temperatura da carcaca do motor: DHT22 + offset didatico (ver config.h)
inline bool lerTemperatura(float &saida) {
    float t = dht.readTemperature();
    if (isnan(t)) return false;
    saida = t + TEMP_OFFSET;
    return true;
}

// Umidade da casa de bombas (umidade alta = possivel vazamento)
inline bool lerUmidade(float &saida) {
    float u = dht.readHumidity();
    if (isnan(u)) return false;
    saida = u;
    return true;
}
#endif

#if defined(SENSOR_SOM)
// HW-484 (microfone + LM393, saida analogica): amostra ~50 ms do sinal,
// calcula o RMS em torno da media e mapeia a amplitude para uma escala
// didatica de vibracao em mm/s. Cavitacao e desgaste de rolamento sao
// fenomenos acusticos - o microfone captura essa assinatura da bomba.
inline float lerVibracao() {
    const int N = 256;
    uint32_t soma = 0;
    uint16_t amostras[N];
    for (int i = 0; i < N; i++) {
        amostras[i] = analogRead(PINO_ANALOGICO);
        soma += amostras[i];
        delayMicroseconds(200); // ~51 ms no total
    }
    float media = soma / (float)N;
    float somaQuad = 0;
    for (int i = 0; i < N; i++) {
        float d = amostras[i] - media;
        somaQuad += d * d;
    }
    float rms = sqrtf(somaQuad / N); // 0..~2048
    // Calibracao didatica: silencio ~1 mm/s; saturacao ~10 mm/s
    float vib = 1.0f + (rms / 2048.0f) * 9.0f;
    return vib > 10.0f ? 10.0f : vib;
}
#endif

#if defined(SENSOR_MQ)
// MQ-2/MQ-135: gas/qualidade do ar da casa de bombas (espaco confinado),
// em % do fundo de escala do ADC.
// Obs.: o elemento sensor precisa de ~2 min de aquecimento apos ligar.
inline float lerGas() {
    uint32_t soma = 0;
    for (int i = 0; i < 16; i++) {
        soma += analogRead(PINO_ANALOGICO);
        delay(2);
    }
    float media = soma / 16.0f;
    return (media / 4095.0f) * 100.0f;
}
#endif

#if defined(SENSOR_VAZAO)
// Sensor de vazao hall (YF-S201 ou similar): a turbina gera pulsos com
// frequencia proporcional a vazao. Contamos os pulsos por interrupcao e
// convertemos em L/min na janela de publicacao.
// Atencao: alimentar em 5 V e trazer o sinal para 3,3 V com divisor resistivo.
static volatile uint32_t pulsosVazao = 0;

void IRAM_ATTR aoDetectarPulsoVazao() { pulsosVazao++; }

inline float lerVazao(unsigned long janelaMs) {
    if (janelaMs == 0) return 0.0f;
    noInterrupts();
    uint32_t pulsos = pulsosVazao;
    pulsosVazao = 0;
    interrupts();
    float hz = (pulsos * 1000.0f) / (float)janelaMs;
    return hz / FATOR_VAZAO_HZ_POR_LMIN;
}
#endif

inline void iniciarSensores() {
#if defined(SENSOR_DHT22)
    dht.begin();
#endif
#if defined(SENSOR_SOM) || defined(SENSOR_MQ)
    analogReadResolution(12);
#endif
#if defined(SENSOR_VAZAO)
    pinMode(PINO_VAZAO, INPUT);
    attachInterrupt(digitalPinToInterrupt(PINO_VAZAO), aoDetectarPulsoVazao, RISING);
#endif
}
