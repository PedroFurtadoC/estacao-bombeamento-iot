// ============================================================
// Mini Central de Monitoramento IoT - no de aquisicao (ESP32)
//
// Le o sensor fisico da maquina (build flag), simula os demais
// sinais, classifica o status no edge (LEDs), mostra a leitura no
// monitor serial e publica o JSON via MQTT no topico
// fabrica/maquinas/<ID>/telemetria.
//
// Roda em LOLIN S2 Mini (M01, M02) e ESP32 DevKit (M03); o mapa
// de pinos de cada placa esta em config.h.
//
// A rede nunca bloqueia a coleta: sem Wi-Fi ou sem broker os
// sensores continuam sendo lidos e impressos, e a publicacao
// acontece assim que o MQTT conecta.
//
// -D MODO_BANCADA=1 (environments *_bancada): desliga Wi-Fi/MQTT
// e imprime a leitura dos sensores a cada 2 s, com os valores
// brutos, para conferir a montagem antes de ligar a rede.
// ============================================================

#include <Arduino.h>
#include <ArduinoJson.h>
#include <math.h>

#include "config.h"
#include "sensores.h"
#include "simulacao.h"

#ifndef MODO_BANCADA
#define MODO_BANCADA 0
#endif

#if !MODO_BANCADA
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "secrets.h"
#endif

#ifndef MACHINE_ID
#define MACHINE_ID "M00"
#endif

#define INTERVALO_BANCADA_MS 2000UL // DHT22 nao le mais rapido que isso
static const unsigned long INTERVALO_MS = MODO_BANCADA ? INTERVALO_BANCADA_MS : INTERVALO_PUBLICACAO_MS;

#if !MODO_BANCADA
static const char *TOPICO_TELEMETRIA = "fabrica/maquinas/" MACHINE_ID "/telemetria";
static const char *TOPICO_STATUS = "fabrica/maquinas/" MACHINE_ID "/status";
WiFiClient rede;
PubSubClient mqtt(rede);
#endif
EstadoSimulacao sim;
unsigned long ultimaLeitura = 0;
uint32_t contadorLeituras = 0;

// Uma leitura completa da maquina: sinais publicados + valores brutos
// dos sensores, para conferencia no serial.
struct Leitura {
    // sinais do contrato (reais ou simulados)
    float temperatura = 0, vibracao = 0, corrente = 0, rotacao = 0, vazao = 0;
    float umidade = NAN, gas = NAN;
    int status = 0;
    bool anomaliaSimulada = false;
    // brutos dos sensores fisicos
    bool dhtOk = false;
    float tempBruta = NAN;   // DHT22 sem o TEMP_OFFSET
    uint16_t gasAdc = 0;     // MQ: media em contagens do ADC (0..4095)
    uint32_t pulsos = 0;     // vazao: pulsos contados na janela
    unsigned long janelaMs = 0;
};

// ---------------- Classificacao de status (edge) ----------------
// 0 = normal | 1 = atencao | 2 = critico

int classificarTemperatura(float v) {
    if (v > TEMP_CRITICO) return 2;
    if (v > TEMP_ATENCAO) return 1;
    return 0;
}

int classificarVibracao(float v) {
    if (v > VIB_CRITICO) return 2;
    if (v > VIB_ATENCAO) return 1;
    return 0;
}

int classificarCorrente(float v) {
    if (v > CORR_CRITICO) return 2;
    if (v > CORR_ATENCAO) return 1;
    return 0;
}

int classificarRotacao(float v) {
    if (v < ROT_CRITICO_MIN || v > ROT_CRITICO_MAX) return 2;
    if (v < ROT_ATENCAO_MIN || v > ROT_ATENCAO_MAX) return 1;
    return 0;
}

int classificarGas(float v) {
    if (v > GAS_CRITICO) return 2;
    if (v > GAS_ATENCAO) return 1;
    return 0;
}

int classificarVazao(float v) {
    if (v < VAZAO_CRITICO_MIN || v > VAZAO_CRITICO_MAX) return 2;
    if (v < VAZAO_ATENCAO_MIN || v > VAZAO_ATENCAO_MAX) return 1;
    return 0;
}

// ---------------- Atuadores de borda ----------------

void iniciarAtuadores() {
    pinMode(PINO_LED_ONBOARD, OUTPUT);
    digitalWrite(PINO_LED_ONBOARD, LOW);
#if defined(ATUADOR_LED_RGB)
    pinMode(PINO_LED_R, OUTPUT);
    pinMode(PINO_LED_G, OUTPUT);
    pinMode(PINO_LED_B, OUTPUT);
#elif defined(ATUADOR_LED_FLASH)
    pinMode(PINO_LED_FLASH, OUTPUT);
#endif
}

void sinalizarStatus(int status) {
#if defined(ATUADOR_LED_RGB)
    // Semaforo: verde = normal, amarelo = atencao, vermelho = critico
    digitalWrite(PINO_LED_R, status >= 1 ? HIGH : LOW);
    digitalWrite(PINO_LED_G, status <= 1 ? HIGH : LOW);
    digitalWrite(PINO_LED_B, LOW);
#elif defined(ATUADOR_LED_FLASH)
    // LED flash 7 cores: energizado apenas em condicao critica
    digitalWrite(PINO_LED_FLASH, status == 2 ? HIGH : LOW);
#else
    (void)status;
#endif
}

// ---------------- Coleta ----------------

// Avanca a simulacao e le os sensores fisicos da maquina.
void coletar(Leitura &l, unsigned long janelaMs) {
    l.janelaMs = janelaMs;
    l.anomaliaSimulada = emJanelaDeAnomalia(millis());
    avancarSimulacao(sim, l.anomaliaSimulada);

    // Valores partem da simulacao; sensores fisicos sobrescrevem o seu sinal
    l.temperatura = sim.temperatura;
    l.vibracao = sim.vibracao;
    l.corrente = sim.corrente;
    l.rotacao = sim.rotacao;
    l.vazao = sim.vazao;

#if defined(SENSOR_DHT22)
    float t, u;
    l.dhtOk = lerDHT(t, u);
    if (l.dhtOk) {
        l.tempBruta = t;
        l.temperatura = t + TEMP_OFFSET;
        l.umidade = u;
    }
#endif
#if defined(SENSOR_MQ)
    l.gas = lerGas(&l.gasAdc);
#endif
#if defined(SENSOR_VAZAO)
    l.vazao = lerVazao(janelaMs, &l.pulsos);
#endif

    int status = classificarTemperatura(l.temperatura);
    status = max(status, classificarVibracao(l.vibracao));
    status = max(status, classificarCorrente(l.corrente));
    status = max(status, classificarRotacao(l.rotacao));
#if defined(SENSOR_MQ)
    status = max(status, classificarGas(l.gas));
#endif
#if !defined(SENSOR_VAZAO) || VAZAO_AFETA_STATUS
    // Vazao simulada sempre conta; a real so quando ha agua circulando
    // (VAZAO_AFETA_STATUS=1), para nao alarmar em bancada seca.
    status = max(status, classificarVazao(l.vazao));
#endif
    l.status = status;
}

// Mostra no serial o que cada sensor fisico entregou (bruto e processado)
// e o que foi simulado. E o que se confere na bancada.
void imprimirLeitura(const Leitura &l) {
    static const char *NOME_STATUS[] = {"normal", "atencao", "critico"};
    Serial.printf("---- leitura #%lu  %s  janela %lu ms ----\n",
                  (unsigned long)contadorLeituras, MACHINE_ID, l.janelaMs);
#if defined(SENSOR_DHT22)
    if (l.dhtOk) {
        Serial.printf("[DHT22 GPIO%d]  %.1f C ambiente (+%.0f offset = %.1f C)  |  umidade %.1f %%\n",
                      PINO_DHT, l.tempBruta, (double)TEMP_OFFSET, l.temperatura, l.umidade);
    } else {
        Serial.printf("[DHT22 GPIO%d]  FALHA (NaN): conferir VCC=3V3, DATA no GPIO %d e pull-up\n",
                      PINO_DHT, PINO_DHT);
    }
#endif
#if defined(SENSOR_VAZAO)
    Serial.printf("[VAZAO GPIO%d]  %lu pulsos em %lu ms = %.1f Hz  ->  %.2f L/min (K=%.1f Hz por L/min)\n",
                  PINO_VAZAO, (unsigned long)l.pulsos, l.janelaMs,
                  l.janelaMs ? l.pulsos * 1000.0f / l.janelaMs : 0.0f, l.vazao,
                  (double)FATOR_VAZAO_HZ_POR_LMIN);
#endif
#if defined(SENSOR_MQ)
    Serial.printf("[MQ GPIO%d]  adc %u/4095 (~%.2f V no pino, ~%.2f V no A0)  ->  gas %.1f %%\n",
                  PINO_MQ_AO, l.gasAdc, l.gasAdc * 3.3f / 4095.0f,
                  l.gasAdc * 3.3f / 4095.0f * 1.5f, l.gas);
#endif
    Serial.printf("[simulado]  temp %.1f C%s  vib %.2f mm/s%s  corrente %.1f A  rotacao %.0f RPM  vazao %.1f L/min%s%s\n",
                  l.temperatura,
#if defined(SENSOR_DHT22)
                  l.dhtOk ? " (real)" : "",
#else
                  "",
#endif
                  l.vibracao, "",
                  l.corrente, l.rotacao, l.vazao,
#if defined(SENSOR_VAZAO)
                  " (real)",
#else
                  "",
#endif
                  l.anomaliaSimulada ? "  [janela de ANOMALIA simulada]" : "");
    Serial.printf("[status]  %d (%s)\n", l.status, NOME_STATUS[l.status]);
}

// ---------------- Conectividade (nao bloqueante) ----------------
#if !MODO_BANCADA

// Mantem Wi-Fi e MQTT no ar sem travar o loop: cada chamada tenta no
// maximo um passo (iniciar Wi-Fi, ou uma conexao MQTT) e volta.
void manterRede() {
    static bool wifiIniciado = false;
    static bool wifiEstavaConectado = false;
    static unsigned long ultimaTentativaWifi = 0;
    static unsigned long ultimaTentativaMqtt = 0;
    unsigned long agora = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (wifiEstavaConectado) {
            Serial.println("[wifi] conexao perdida, reconectando");
            wifiEstavaConectado = false;
        }
        if (!wifiIniciado || agora - ultimaTentativaWifi >= 15000UL) {
            Serial.printf("[wifi] conectando a %s ...\n", WIFI_SSID);
            WiFi.mode(WIFI_STA);
            WiFi.setAutoReconnect(true);
            WiFi.begin(WIFI_SSID, WIFI_SENHA);
            wifiIniciado = true;
            ultimaTentativaWifi = agora;
        }
        return;
    }

    if (!wifiEstavaConectado) {
        wifiEstavaConectado = true;
        Serial.printf("[wifi] conectado, IP %s\n", WiFi.localIP().toString().c_str());
        configTime(FUSO_SEGUNDOS, 0, SERVIDOR_NTP_1, SERVIDOR_NTP_2);
    }

    if (!mqtt.connected() && agora - ultimaTentativaMqtt >= 5000UL) {
        ultimaTentativaMqtt = agora;
        String clienteId = String("esp32-") + MACHINE_ID;
        Serial.printf("[mqtt] conectando a %s:%d ... ", MQTT_HOST, MQTT_PORTA);
        // LWT: se o dispositivo cair, o broker publica "offline" (retido)
        if (mqtt.connect(clienteId.c_str(), TOPICO_STATUS, 1, true, "offline")) {
            Serial.println("ok");
            mqtt.publish(TOPICO_STATUS, "online", true);
        } else {
            Serial.printf("falhou (rc=%d), nova tentativa em 5 s\n", mqtt.state());
        }
    }
}

// Timestamp ISO 8601 local (NTP). Retorna false se o relogio ainda nao sincronizou.
bool timestampIso(char *saida, size_t tamanho) {
    struct tm agora;
    if (!getLocalTime(&agora, 100)) return false;
    if (agora.tm_year + 1900 < 2020) return false;
    strftime(saida, tamanho, "%Y-%m-%dT%H:%M:%S", &agora);
    return true;
}

void publicar(const Leitura &l) {
    JsonDocument doc;
    doc["machine"] = MACHINE_ID;
    doc["temperature"] = round(l.temperatura * 10) / 10.0;
    doc["vibration"] = round(l.vibracao * 10) / 10.0;
    doc["current"] = round(l.corrente * 10) / 10.0;
    doc["rotation"] = (int)l.rotacao;
    doc["flow"] = round(l.vazao * 10) / 10.0;
    if (!isnan(l.umidade)) doc["humidity"] = round(l.umidade * 10) / 10.0;
    if (!isnan(l.gas)) doc["gas"] = round(l.gas * 10) / 10.0;
    doc["status"] = l.status;
#if defined(SENSOR_DHT22) || defined(SENSOR_MQ)
    doc["fonte"] = "hibrido";
#else
    doc["fonte"] = "simulado";
#endif
    char ts[24];
    if (timestampIso(ts, sizeof(ts))) doc["timestamp"] = ts;

    char payload[384];
    size_t n = serializeJson(doc, payload, sizeof(payload));

    if (!mqtt.connected()) {
        Serial.printf("[pub] sem MQTT, nao enviado: %s\n", payload);
        return;
    }
    bool ok = mqtt.publish(TOPICO_TELEMETRIA, payload, n);
    Serial.printf("[pub]%s %s\n", ok ? "" : " FALHA", payload);
}
#endif // !MODO_BANCADA

// ---------------- Ciclo principal ----------------

void setup() {
    Serial.begin(115200);
    // No S2 Mini a serial e a USB nativa (CDC): espera ate 3 s o PC abrir a
    // porta para nao perder as primeiras linhas. Na DevKit passa direto.
    unsigned long inicio = millis();
    while (!Serial && millis() - inicio < 3000) delay(10);
    Serial.printf("\n[boot] no %s%s\n", MACHINE_ID, MODO_BANCADA ? " - MODO BANCADA (sem rede)" : "");
    Serial.printf("[boot] pinos: DHT=%d vazao=%d MQ=%d | intervalo %lu ms\n",
                  PINO_DHT, PINO_VAZAO, PINO_MQ_AO, INTERVALO_MS);
    randomSeed(esp_random());

    iniciarSensores();
    iniciarAtuadores();
    sinalizarStatus(0);

#if !MODO_BANCADA
    mqtt.setServer(MQTT_HOST, MQTT_PORTA);
    mqtt.setBufferSize(512);
    manterRede();
#endif
    ultimaLeitura = millis();
}

void loop() {
#if !MODO_BANCADA
    manterRede();
    mqtt.loop();
#endif

    unsigned long agora = millis();
    if (agora - ultimaLeitura < INTERVALO_MS) return;
    unsigned long janela = agora - ultimaLeitura;
    ultimaLeitura = agora;
    contadorLeituras++;

    Leitura l;
    coletar(l, janela);
    sinalizarStatus(l.status);

    digitalWrite(PINO_LED_ONBOARD, HIGH); // piscada curta = leitura feita
    imprimirLeitura(l);
#if !MODO_BANCADA
    publicar(l);
#endif
    digitalWrite(PINO_LED_ONBOARD, LOW);
}
