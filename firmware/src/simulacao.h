#pragma once

#include <Arduino.h>
#include "config.h"

// ============================================================
// Simulacao dos sinais sem sensor fisico (Desafio 2):
// variacao normal + pequenas oscilacoes (random walk limitado)
// e janela periodica de anomalia levando os sinais a faixa critica.
// ============================================================

struct EstadoSimulacao {
    float temperatura = 70.0f; // C     (normal 65-75)
    float vibracao = 2.2f;     // mm/s  (normal 1.0-3.5)
    float corrente = 7.5f;     // A     (normal 6.0-9.0)
    float rotacao = 3500.0f;   // RPM   (motobomba 2 polos, normal 3400-3600)
    float vazao = 30.0f;       // L/min (normal 20-40)
};

// Passo aleatorio pequeno, limitado a [minimo, maximo]
inline float passeioAleatorio(float atual, float passo, float minimo, float maximo) {
    float delta = (random(-1000, 1001) / 1000.0f) * passo;
    float novo = atual + delta;
    if (novo < minimo) novo = minimo;
    if (novo > maximo) novo = maximo;
    return novo;
}

// Janela de anomalia periodica (relogio do dispositivo)
inline bool emJanelaDeAnomalia(unsigned long agoraMs) {
    unsigned long fase = agoraMs % SIM_ANOMALIA_PERIODO_MS;
    // A janela abre no fim de cada periodo para dar tempo de acumular dados normais
    return fase >= (SIM_ANOMALIA_PERIODO_MS - SIM_ANOMALIA_DURACAO_MS);
}

// Avanca um passo da simulacao. Durante a anomalia, temperatura e corrente
// sobem juntas (correlacao fisica: mais carga -> mais corrente -> mais calor)
// e a vibracao acompanha.
inline void avancarSimulacao(EstadoSimulacao &s, bool anomalia) {
    if (anomalia) {
        s.temperatura = passeioAleatorio(s.temperatura + 1.5f, 0.8f, 78.0f, 86.0f);
        s.corrente = passeioAleatorio(s.corrente + 0.5f, 0.4f, 10.0f, 13.0f);
        s.vibracao = passeioAleatorio(s.vibracao + 0.4f, 0.3f, 4.0f, 7.0f);
        s.rotacao = passeioAleatorio(s.rotacao - 25.0f, 30.0f, 3150.0f, 3380.0f);
        s.vazao = passeioAleatorio(s.vazao - 2.0f, 1.5f, 8.0f, 18.0f);
    } else {
        s.temperatura = passeioAleatorio(s.temperatura, 0.6f, 65.0f, 75.0f);
        s.vibracao = passeioAleatorio(s.vibracao, 0.25f, 1.0f, 3.4f);
        s.corrente = passeioAleatorio(s.corrente, 0.3f, 6.0f, 8.9f);
        s.rotacao = passeioAleatorio(s.rotacao, 25.0f, 3420.0f, 3580.0f);
        s.vazao = passeioAleatorio(s.vazao, 1.0f, 24.0f, 38.0f);
    }
}
