# Firmware das ESP32

Projeto PlatformIO único: o mesmo código atende as 3 máquinas. A identidade
(`MACHINE_ID`) e os sensores/atuadores de cada uma são definidos por build
flags em `platformio.ini`.

## Preparação

```powershell
Copy-Item src/secrets.h.example src/secrets.h   # preencher Wi-Fi e IP do broker
pio run                                          # compila os 3 environments
pio run -e maquina01 -t upload                   # grava (repetir p/ maquina02/03)
pio device monitor                               # monitor serial 115200
```

## Ligações (wiring)

### Máquina 01: DHT22 + vazão (hall) + LED RGB (HW-479)

| Módulo | Pino módulo | ESP32 |
|---|---|---|
| DHT22/AM2302 | VCC / DATA / GND | 3V3 / GPIO 4 / GND |
| Vazão hall (YF-S201 ou similar) | VCC / GND / SINAL | **5V (VIN)** / GND / GPIO 33 (via divisor, ver abaixo) |
| HW-479 (RGB) | R / G / B / - | GPIO 25 / GPIO 26 / GPIO 27 / GND |

### Máquina 02: DHT22 + HW-484 (som/vibração)

| Módulo | Pino módulo | ESP32 |
|---|---|---|
| DHT22/AM2302 | VCC / DATA / GND | 3V3 / GPIO 4 / GND |
| HW-484 | + / G / A0 | 3V3 / GND / GPIO 34 |

> Usar a saída **A0** (analógica). GPIO 34 é somente-entrada (ADC1), ideal para isso.

### Máquina 03: DHT22 + MQ + vazão (hall) + LED flash (HW-481)

| Módulo | Pino módulo | ESP32 |
|---|---|---|
| DHT22/AM2302 | VCC / DATA / GND | 3V3 / GPIO 4 / GND |
| MQ (Flying-Fish) | VCC / GND / A0 | **5V (VIN)** / GND / GPIO 34 (via divisor) |
| Vazão hall (YF-S201 ou similar) | VCC / GND / SINAL | **5V (VIN)** / GND / GPIO 33 (via divisor) |
| HW-481 (flash) | S / - | GPIO 25 / GND |

> O sensor MQ (MQ-2 ou MQ-135, conferir o código no cilindro metálico) tem
> aquecedor interno: alimente em 5 V e aguarde cerca de 2 min antes de confiar
> na leitura. Na demonstração, gás de isqueiro sem acender (MQ-2) ou álcool
> (MQ-135) faz a leitura subir rapidamente.

### Divisores de tensão (obrigatórios nos dois sensores de 5 V)

O ESP32 aceita no máximo 3,3 V nos GPIOs. Tanto a saída A0 do MQ quanto o
sinal do sensor de vazão são referenciados aos 5 V da alimentação e podem
ultrapassar esse limite, então **cada um precisa do seu divisor**:

```
Saída do sensor (5 V) ---[ 10 kΩ ]---+--- GPIO do ESP32
                                     |
                                  [ 20 kΩ ]
                                     |
                                    GND
```

Com 10 kΩ e 20 kΩ o pico de 5 V chega ao GPIO como 3,3 V. Valem quaisquer
resistores na proporção 1:2 (por exemplo 1 kΩ e 2 kΩ). O GND do ESP32, do
sensor e do divisor precisa ser o mesmo.

> **Alimentação da M03**: o aquecedor do MQ consome cerca de 150 mA. Alimente
> a placa por uma porta USB boa do notebook ou por um carregador de 5 V com
> 1 A ou mais, senão o Wi-Fi pode reiniciar durante os picos de transmissão.

> **Vazão em bancada seca**: sem água a leitura fica em 0 L/min (comportamento
> correto de bomba a seco); soprar na turbina gera vazão para a demonstração.
> Conversão padrão YF-S201 (7,5 Hz por L/min), ajustável em
> `FATOR_VAZAO_HZ_POR_LMIN`.

## Comportamento

- Publica JSON a cada 10 s em `fabrica/maquinas/<ID>/telemetria`;
- Sinais sem sensor físico são simulados (variação normal + anomalia periódica
  a cada 15 min, Desafio 2);
- Status calculado no edge (0/1/2) aciona LED RGB (M01) e alarme (M03);
- Vazão real (M01/M03) é publicada sempre, mas só entra no status com
  `-D VAZAO_AFETA_STATUS=1` (evita alarme permanente em bancada seca);
- LWT: queda do dispositivo publica `offline` retido no tópico `status`;
- Timestamp via NTP (UTC-3); sem sincronismo, o back-end usa a hora de chegada;
- `TEMP_OFFSET` (padrão +45 °C) leva a leitura ambiente do DHT22 à faixa do
  cenário; aquecer o sensor com o dedo dispara o alerta ao vivo.
