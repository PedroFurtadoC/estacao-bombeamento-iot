// ============================================================
// Mini Central de Monitoramento IoT - no de aquisicao (ESP32)
//
// Le o sensor fisico da maquina (build flag), simula os demais
// sinais, classifica o status no edge (LEDs) e publica o JSON
// via MQTT no topico fabrica/maquinas/<ID>/telemetria.
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

#include "config.h"
#include "secrets.h"
#include "sensores.h"
#include "simulacao.h"

#ifndef MACHINE_ID
#define MACHINE_ID "M00"
#endif

static const char *TOPICO_TELEMETRIA = "fabrica/maquinas/" MACHINE_ID "/telemetria";
static const char *TOPICO_STATUS = "fabrica/maquinas/" MACHINE_ID "/status";

WiFiClient rede;
PubSubClient mqtt(rede);
EstadoSimulacao sim;
unsigned long ultimaPublicacao = 0;

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

// ---------------- Conectividade ----------------

void conectarWifi() {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.printf("[wifi] conectando a %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_SENHA);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[wifi] conectado, IP %s\n", WiFi.localIP().toString().c_str());
}

void conectarMqtt() {
    while (!mqtt.connected()) {
        String clienteId = String("esp32-") + MACHINE_ID;
        Serial.printf("[mqtt] conectando a %s:%d... ", MQTT_HOST, MQTT_PORTA);
        // LWT: se o dispositivo cair, o broker publica "offline" (retido)
        if (mqtt.connect(clienteId.c_str(), TOPICO_STATUS, 1, true, "offline")) {
            Serial.println("ok");
            mqtt.publish(TOPICO_STATUS, "online", true);
        } else {
            Serial.printf("falhou (rc=%d), nova tentativa em 3 s\n", mqtt.state());
            delay(3000);
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

// ---------------- Ciclo principal ----------------

void publicarTelemetria() {
    bool anomalia = emJanelaDeAnomalia(millis());
    avancarSimulacao(sim, anomalia);

    // Valores partem da simulacao; sensores fisicos sobrescrevem o seu sinal
    float temperatura = sim.temperatura;
    float vibracao = sim.vibracao;
    float corrente = sim.corrente;
    float rotacao = sim.rotacao;
    float vazao = sim.vazao;

    JsonDocument doc;
    doc["machine"] = MACHINE_ID;

#if defined(SENSOR_DHT22)
    float tReal, uReal;
    if (lerTemperatura(tReal)) temperatura = tReal;
    if (lerUmidade(uReal)) doc["humidity"] = round(uReal * 10) / 10.0;
#endif
#if defined(SENSOR_SOM)
    vibracao = lerVibracao();
#endif
#if defined(SENSOR_MQ)
    float gas = lerGas();
    doc["gas"] = round(gas * 10) / 10.0;
#endif
#if defined(SENSOR_VAZAO)
    vazao = lerVazao(INTERVALO_PUBLICACAO_MS);
#endif

    int status = classificarTemperatura(temperatura);
    status = max(status, classificarVibracao(vibracao));
    status = max(status, classificarCorrente(corrente));
    status = max(status, classificarRotacao(rotacao));
#if defined(SENSOR_MQ)
    status = max(status, classificarGas(gas));
#endif
#if !defined(SENSOR_VAZAO) || VAZAO_AFETA_STATUS
    // Vazao simulada sempre conta; a real so quando ha agua circulando
    // (VAZAO_AFETA_STATUS=1), para nao alarmar em bancada seca.
    status = max(status, classificarVazao(vazao));
#endif

    sinalizarStatus(status);

    doc["temperature"] = round(temperatura * 10) / 10.0;
    doc["vibration"] = round(vibracao * 10) / 10.0;
    doc["current"] = round(corrente * 10) / 10.0;
    doc["rotation"] = (int)rotacao;
    doc["flow"] = round(vazao * 10) / 10.0;
    doc["status"] = status;
#if defined(SENSOR_DHT22) || defined(SENSOR_SOM) || defined(SENSOR_MQ)
    doc["fonte"] = "hibrido";
#else
    doc["fonte"] = "simulado";
#endif

    char ts[24];
    if (timestampIso(ts, sizeof(ts))) doc["timestamp"] = ts;

    char payload[384];
    size_t n = serializeJson(doc, payload, sizeof(payload));
    bool ok = mqtt.publish(TOPICO_TELEMETRIA, payload, n);
    Serial.printf("[pub]%s %s\n", ok ? "" : " FALHA", payload);
}

void setup() {
    Serial.begin(115200);
    randomSeed(esp_random());

    iniciarSensores();
    iniciarAtuadores();
    sinalizarStatus(0);

    conectarWifi();
    configTime(FUSO_SEGUNDOS, 0, SERVIDOR_NTP_1, SERVIDOR_NTP_2);

    mqtt.setServer(MQTT_HOST, MQTT_PORTA);
    mqtt.setBufferSize(512);
    conectarMqtt();
}

void loop() {
    conectarWifi();
    conectarMqtt();
    mqtt.loop();

    unsigned long agora = millis();
    if (agora - ultimaPublicacao >= INTERVALO_PUBLICACAO_MS || ultimaPublicacao == 0) {
        ultimaPublicacao = agora;
        publicarTelemetria();
    }
}
