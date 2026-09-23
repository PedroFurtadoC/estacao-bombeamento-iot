// No de aquisicao da estacao de bombeamento (ESP32).
// Le o sensor da maquina, simula os sinais que nao tem sensor, classifica o
// status ali mesmo (LEDs) e publica em fabrica/maquinas/<ID>/telemetria.
// Roda em DevKit (M01, M03) e ESP32-S3 (M02); os pinos estao em config.h.
//
// A rede nunca segura a coleta: sem Wi-Fi ou sem broker os sensores continuam
// sendo lidos, e o que nao pode ser publicado fica numa fila curta.
//
// MODO_BANCADA=1 (environments *_bancada) desliga Wi-Fi/MQTT e imprime tudo a
// cada 2 s, para conferir a montagem antes de ligar a rede.

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
#if !__has_include("secrets.h")
#error "Falta src/secrets.h. Copie src/secrets.h.example e preencha Wi-Fi e broker (ou grave um environment *_bancada, que nao usa rede)."
#endif
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
#define TAM_PAYLOAD 320 // o JSON cheio da ~180
WiFiClient rede;
PubSubClient mqtt(rede);
#endif
EstadoSimulacao sim;
unsigned long ultimaLeitura = 0;
uint32_t contadorLeituras = 0;

struct Leitura {
    // sinais do contrato, reais ou simulados
    float temperatura = 0, vibracao = 0, corrente = 0, rotacao = 0, vazao = 0;
    float umidade = NAN, gas = NAN;
    int status = 0;
    bool anomaliaSimulada = false;
    // brutos, so para o serial
    bool dhtOk = false;
    float tempBruta = NAN;   // sem o TEMP_OFFSET
    uint16_t gasAdc = 0;     // 0..4095
    uint32_t pulsos = 0;     // na janela
    unsigned long janelaMs = 0;
};

// ---------------- Classificacao na borda (0 normal, 1 atencao, 2 critico) --

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

// ---------------- Atuadores ----------------

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
    // verde = normal, amarelo = atencao, vermelho = critico
    digitalWrite(PINO_LED_R, status >= 1 ? HIGH : LOW);
    digitalWrite(PINO_LED_G, status <= 1 ? HIGH : LOW);
    digitalWrite(PINO_LED_B, LOW);
#elif defined(ATUADOR_LED_FLASH)
    digitalWrite(PINO_LED_FLASH, status == 2 ? HIGH : LOW); // so no critico
#else
    (void)status;
#endif
}

// ---------------- Coleta ----------------

void coletar(Leitura &l, unsigned long janelaMs) {
    l.janelaMs = janelaMs;
    l.anomaliaSimulada = emJanelaDeAnomalia(millis());
    avancarSimulacao(sim, l.anomaliaSimulada);

    // parte da simulacao; o sensor fisico sobrescreve o sinal dele
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
    // a vazao real so conta com agua circulando (ver config.h)
    status = max(status, classificarVazao(l.vazao));
#endif
    l.status = status;
}

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

// ---------------- Rede ----------------
#if !MODO_BANCADA

// Cada chamada tenta no maximo um passo (subir o Wi-Fi ou conectar o MQTT) e
// devolve o controle, para o loop nunca travar esperando rede.
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
        // LWT: se a placa cair, o broker anuncia "offline" por ela
        if (mqtt.connect(clienteId.c_str(), TOPICO_STATUS, 1, true, "offline")) {
            Serial.println("ok");
            mqtt.publish(TOPICO_STATUS, "online", true);
        } else {
            Serial.printf("falhou (rc=%d), nova tentativa em 5 s\n", mqtt.state());
        }
    }
}

// false enquanto o NTP nao sincronizou.
bool timestampIso(char *saida, size_t tamanho) {
    struct tm agora;
    if (!getLocalTime(&agora, 100)) return false;
    if (agora.tm_year + 1900 < 2020) return false;
    strftime(saida, tamanho, "%Y-%m-%dT%H:%M:%S", &agora);
    return true;
}

// Devolve o tamanho do JSON e avisa se ele saiu com timestamp.
size_t montarPayload(const Leitura &l, char *destino, size_t tamanho, bool &comTimestamp) {
    JsonDocument doc;
    doc["machine"] = MACHINE_ID;
    doc["temperature"] = round(l.temperatura * 10) / 10.0;
    doc["vibration"] = round(l.vibracao * 10) / 10.0;
    doc["current"] = round(l.corrente * 10) / 10.0;
    doc["rotation"] = (int)lroundf(l.rotacao); // lround, nao truncar: o serial
                                               // usa %.0f e os dois tem que bater
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
    comTimestamp = timestampIso(ts, sizeof(ts));
    if (comTimestamp) doc["timestamp"] = ts;

    return serializeJson(doc, destino, tamanho);
}

// O cast e obrigatorio: com (char*, n) o compilador escolhe
// publish(topico, const char*, boolean retained) e o tamanho vira
// retained=true, fazendo o broker guardar e reentregar leitura velha.
bool enviarTelemetria(const char *payload, size_t n) {
    return mqtt.publish(TOPICO_TELEMETRIA, (const uint8_t *)payload, (unsigned int)n, false);
}

// Fila para a queda de rede nao abrir buraco na serie. Como cada leitura leva
// o proprio timestamp, o que fica parado aqui entra no banco na hora certa.
// So entra leitura com timestamp: sem NTP o back-end usaria a hora de chegada.
#define PENDENTES_MAX 6 // 1 minuto a cada 10 s
static char pendentes[PENDENTES_MAX][TAM_PAYLOAD];
static size_t pendentesTam[PENDENTES_MAX];
static uint8_t pendentesInicio = 0, pendentesTotal = 0;

void guardarPendente(const char *payload, size_t n) {
    uint8_t pos = (pendentesInicio + pendentesTotal) % PENDENTES_MAX;
    if (pendentesTotal == PENDENTES_MAX) { // cheia: descarta a mais velha
        pendentesInicio = (pendentesInicio + 1) % PENDENTES_MAX;
    } else {
        pendentesTotal++;
    }
    memcpy(pendentes[pos], payload, n);
    pendentesTam[pos] = n;
}

void enviarPendentes() {
    while (pendentesTotal > 0) {
        if (!enviarTelemetria(pendentes[pendentesInicio], pendentesTam[pendentesInicio])) return;
        pendentesInicio = (pendentesInicio + 1) % PENDENTES_MAX;
        pendentesTotal--;
    }
}

void publicar(const Leitura &l) {
    char payload[TAM_PAYLOAD];
    bool comTimestamp = false;
    size_t n = montarPayload(l, payload, sizeof(payload), comTimestamp);

    // truncado viraria JSON invalido e a ingestao descartaria calado
    if (n == 0 || n >= sizeof(payload) - 1) {
        Serial.printf("[pub] payload nao coube em %u bytes, leitura descartada\n", (unsigned)sizeof(payload));
        return;
    }

    if (!mqtt.connected()) {
        if (comTimestamp) {
            guardarPendente(payload, n);
            Serial.printf("[pub] sem MQTT, guardado (%u na fila): %s\n", pendentesTotal, payload);
        } else {
            Serial.printf("[pub] sem MQTT e sem NTP, descartado: %s\n", payload);
        }
        return;
    }

    if (pendentesTotal > 0) {
        uint8_t antes = pendentesTotal;
        enviarPendentes();
        Serial.printf("[pub] fila drenada: %u de %u\n", antes - pendentesTotal, antes);
    }
    bool ok = enviarTelemetria(payload, n);
    if (!ok && comTimestamp) guardarPendente(payload, n);
    Serial.printf("[pub]%s %s\n", ok ? "" : " FALHA", payload);
}
#endif // !MODO_BANCADA

// ---------------- Ciclo principal ----------------

void setup() {
    Serial.begin(115200);
    // placas de USB nativa: espera ate 3 s o PC abrir a porta, senao as
    // primeiras linhas se perdem. Na DevKit e no S3 passa direto.
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
    // sem isso o connect() prende ate 15 s quando o IP responde mas nao ha
    // broker na porta, e a janela de contagem da vazao estica junto
    mqtt.setSocketTimeout(5);
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

    digitalWrite(PINO_LED_ONBOARD, HIGH); // piscada = leitura feita
    imprimirLeitura(l);
#if !MODO_BANCADA
    publicar(l);
#endif
    digitalWrite(PINO_LED_ONBOARD, LOW);
}
