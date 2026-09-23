# 03 - Modelo de Dados

## Tópicos MQTT

| Tópico | Direção | Payload |
|---|---|---|
| `fabrica/maquinas/M01/telemetria` | ESP32 → broker | JSON de telemetria |
| `fabrica/maquinas/M02/telemetria` | ESP32 → broker | JSON de telemetria |
| `fabrica/maquinas/M03/telemetria` | ESP32 → broker | JSON de telemetria |
| `fabrica/maquinas/<ID>/status` | ESP32 → broker (LWT) | `online` / `offline` |

A ingestão assina o curinga `fabrica/maquinas/+/telemetria`.

## Payload de telemetria (contrato)

```json
{
  "machine": "M01",
  "temperature": 72.4,
  "vibration": 2.8,
  "current": 8.2,
  "rotation": 3510,
  "humidity": 41.5,
  "gas": 12.0,
  "status": 0,
  "fonte": "hibrido",
  "timestamp": "2026-08-20T17:00:00"
}
```

| Campo | Tipo | Obrigatório | Descrição |
|---|---|---|---|
| `machine` | string | Sim | `M01`, `M02` ou `M03` |
| `temperature` | float (°C) | Sim | Sinal 1 do enunciado |
| `vibration` | float (mm/s) | Sim | Sinal 2 do enunciado |
| `current` | float (A) | Sim | Sinal 3 do enunciado |
| `rotation` | int (RPM) | Sim | Sinal 4 do enunciado |
| `humidity` | float (%) | opcional | Extra do DHT22: umidade da casa de bombas (alta = indício de vazamento) |
| `gas` | float (%) | opcional | Extra do MQ (M03): gases no espaço confinado |
| `flow` | float (L/min) | opcional | Vazão da motobomba (sensor hall nas M01/M02, simulada na M03) |
| `status` | int | opcional | 0=normal, 1=atenção, 2=crítico (calculado no edge; a ingestão recalcula se ausente) |
| `fonte` | string | opcional | `sensor`, `simulado` ou `hibrido` |
| `timestamp` | string ISO 8601 | opcional | Hora local via NTP; se ausente, a ingestão usa a hora de chegada |

## Faixas operacionais e limiares de alerta

As faixas de temperatura são as do enunciado. As dos outros sinais fomos nós
que definimos, tentando manter coerência com o cenário (motor de 2 polos a
60 Hz, girando por volta de 3500 RPM):

| Sinal | Normal | Atenção | Crítico |
|---|---|---|---|
| Temperatura (°C) | 65-75 | 75-80 | > 80 |
| Vibração (mm/s) | 1,0-3,5 | 3,5-5,0 | > 5,0 |
| Corrente (A) | 6,0-9,0 | 9,0-11,0 | > 11,0 |
| Rotação (RPM) | 3400-3600 | 3300-3400 ou 3600-3700 | < 3300 ou > 3700 |
| Gás (%) | < 45 | 45-60 | > 60 |
| Vazão (L/min) | 20-40 | 10-20 ou 40-45 | < 10 ou > 45 |

Interpretação física: queda de RPM com corrente e temperatura altas indica
sobrecarga/atrito (rolamento); vibração alta indica cavitação; queda de vazão
acompanha a cavitação/obstrução; vazão zero com corrente baixa indica operação
a seco.

> Vazão real em bancada: sem água circulando o sensor lê 0 L/min (leitura
> correta de bomba a seco), o que deixaria a máquina em alerta permanente. Por
> isso o firmware publica a vazão sempre, mas só a inclui no cálculo de status
> com o build flag `VAZAO_AFETA_STATUS=1` (ligar quando houver água na
> demonstração). A vazão simulada (M03) sempre conta no status.

`status` da máquina = pior classificação entre os sinais no instante da leitura.

> Modo demonstração (sensor em bancada): o DHT22 lê a temperatura ambiente,
> não a de um motor. O firmware soma um offset (`TEMP_OFFSET`, padrão +40 °C)
> para levar a leitura à faixa do enunciado; aquecer o sensor com o dedo
> dispara a anomalia ao vivo na apresentação. O offset era +45 até a bancada
> de 21/09: a sala estava em 30 °C, a conta dava 75,1 °C e as máquinas
> nasciam em "atenção" sem nada de errado.

> A faixa de gás é em % do fundo de escala do ADC, não em ppm: sem calibrar o
> MQ com gás de referência, ppm seria um número inventado. Os limiares saíram
> da medida em ar limpo com o sensor já aquecido, 31 % (ADC ~1260 de 4095).
> Com os 20 % que usávamos antes, a M03 ficava permanentemente em atenção.

## Schema no InfluxDB

- **Org:** `unaerp` · **Bucket:** `iot` · **Retenção:** 30 dias
- **Measurement:** `telemetria`

| Elemento | Chaves |
|---|---|
| **Tags** (indexadas) | `machine` (M01/M02/M03), `fonte` (sensor/simulado/hibrido) |
| **Fields** | `temperature`, `vibration`, `current`, `rotation`, `flow`, `humidity`, `gas`, `status_code` |
| **Timestamp** | do payload (NTP) ou da chegada ao back-end |

### Por que tags × fields assim?

`machine` é tag porque toda consulta filtra/agrupa por máquina (indexação);
valores numéricos são fields (não indexáveis, alta cardinalidade). `status_code`
é field numérico para permitir agregações (contagem de máquinas normais e de
eventos de alerta, painéis 6 e 7).

## Retenção (Desafio 4, pergunta 7)

| Dado | Retenção sugerida | Motivo |
|---|---|---|
| Telemetria bruta (10 s) | 30 dias | Diagnóstico recente e detalhado |
| Agregados 1 min/1 h (downsampling) | 1 a 5 anos | Tendência histórica, auditoria, manutenção preditiva |
| Eventos de alerta | anos | Registro de incidentes, o dado mais valioso a longo prazo |

## Volume de dados (Desafio 2)

Com 3 máquinas publicando a cada 10 s, são 18 registros por minuto, ou seja,
**500 registros em cerca de 28 minutos** de operação real. O simulador (`--backfill 2h`) insere ~2160 registros
retroativos instantaneamente, garantindo o requisito com folga.
