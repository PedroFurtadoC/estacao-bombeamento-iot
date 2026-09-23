# Firmware das ESP32

Projeto PlatformIO único: o mesmo código atende as 3 máquinas. A identidade
(`MACHINE_ID`), a placa e os sensores/atuadores de cada uma são definidos por
environment em `platformio.ini`; o mapa de pinos de cada placa fica em
`src/config.h` e é escolhido automaticamente pelo alvo compilado.

## Placas desta montagem

| Máquina | Environment | Placa (`board`) | Sensores | Atuador |
|---|---|---|---|---|
| M01 | `maquina01` | ESP32 DevKit V1 (`esp32dev`) | DHT22 + vazão YF-S201C | LED RGB HW-479 |
| M02 | `maquina02` | ESP32-S3-N16R8, formato DevKitC-1 (`esp32-s3-devkitc-1`) | DHT22 + vazão YF-S402 | nenhum |
| M03 | `maquina03` | ESP32 DevKit V1 (`esp32dev`) | DHT22 + MQ (gás) | LED flash HW-481 |

Para trocar a placa de uma máquina troque só o `extends`/`board` do
environment (`board = esp32dev`, `s3n16r8` ou `s2mini`); os pinos seguem a
tabela da placa, escolhida em `config.h` pelo alvo compilado.

## Preparação

```powershell
Copy-Item src/secrets.h.example src/secrets.h   # preencher Wi-Fi e IP do broker
pio run                                          # compila os 3 environments
pio run -e maquina01 -t upload                   # grava (repetir p/ maquina02/03)
pio device monitor                               # monitor serial 115200
```

> **DevKit (M01 e M03):** grava pela micro-USB com o chip USB-serial da placa
> (CP2102 ou CH340; se o Windows não mostrar a porta COM, instale o driver).
> Algumas DevKit não entram sozinhas em modo de gravação: se o upload travar
> em `Connecting......`, segure o botão **BOOT** até começar a gravar.
>
> **S3-N16R8 (M02):** grave e monitore pela porta USB-C marcada **`UART`** (ou
> `COM`), que tem o chip USB-serial CH343 (`VID:PID=1A86:55D3`): auto-reset,
> sem botão. A porta `USB` vai direto ao chip e não mostra a serial com a
> configuração atual. Flash de 16 MB já configurada; a PSRAM fica desligada
> (o firmware não precisa).
>
> **Se alguma máquina for montada num LOLIN S2 Mini** (base `s2mini` do
> `platformio.ini`): a placa só aparece como porta COM em modo de gravação na
> primeira vez. Com o USB desligado, segure o botão **0**, ligue o USB e
> solte o **0**; `pio device list` deve mostrar `VID:PID=303A:0002`. Depois da
> primeira gravação o upload é automático. O `tools/pio_s2.py` (ativado por
> `extra_scripts` na base `s2mini`) troca o esptool pelo `tools/esptool_s2.py`,
> que trata o `FAILED` falso que o esptool dá ao reiniciar o S2 pela USB nativa.

## Ligações (wiring)

> Esquema completo, com diagrama de cada placa, cálculo dos divisores e como
> adaptar os fios do sensor de vazão para a protoboard:
> [`../docs/10-esquema-eletrico.md`](../docs/10-esquema-eletrico.md).
> Montagem furo a furo na protoboard, uma figura por máquina:
> [`../docs/assets/protoboard.html`](../docs/assets/protoboard.html).

### Máquina 01 (DevKit): DHT22 + vazão YF-S201C + LED RGB (HW-479)

| Módulo | Pino módulo | ESP32 DevKit |
|---|---|---|
| DHT22/AM2302 | VCC / DATA / GND | 3V3 / **GPIO 4** / GND |
| Vazão YF-S201C (1/2") | VCC / GND / SINAL | **VIN (5 V)** / GND / **GPIO 33** (via divisor, ver abaixo) |
| HW-479 (RGB) | R / G / B / - | **GPIO 25 / GPIO 26 / GPIO 27** / GND |

> Na DevKit não use os GPIO 0, 2, 12 e 15 (interferem no boot) nem 6 a 11
> (flash interna). GPIO 34, 35, 36 e 39 são somente entrada, sem pull-up.

### Máquina 02 (ESP32-S3-N16R8): DHT22 + vazão YF-S402

| Módulo | Pino módulo | S3 (DevKitC-1) |
|---|---|---|
| DHT22/AM2302 | VCC / DATA / GND | 3V3 / **GPIO 4** / GND |
| Vazão YF-S402 (1/4") | VCC / GND / SINAL | **3V3** / GND / **GPIO 5** direto, sem divisor (`VAZAO_PULLUP_INTERNO=1`, já no environment) |

No S3 não use os GPIO 35/36/37 (PSRAM octal do R8), 19/20 (USB), 43/44
(serial do monitor), 0/3/45/46 (boot) nem 48 (LED RGB da placa). Todos os
pinos vêm escritos na serigrafia.

É a M01 sem o LED e com a vazão em 3,3 V. Só muda o sensor (e o fator de conversão, já no `platformio.ini`).

### Máquina 03 (DevKit): DHT22 + MQ (gás) + LED flash (HW-481)

| Módulo | Pino módulo | ESP32 DevKit |
|---|---|---|
| DHT22/AM2302 | VCC / DATA / GND | 3V3 / GPIO 4 / GND |
| MQ (Flying-Fish) | VCC / GND / A0 | **VIN (5 V)** / GND / GPIO 34 (via divisor) |
| HW-481 (flash) | S / - | GPIO 25 / GND |

> GPIO 34 é somente-entrada (ADC1), ideal para o analógico. A vibração da M03 é simulada, como nas outras máquinas.

> O sensor MQ (MQ-2 ou MQ-135, conferir o código no cilindro metálico) tem
> aquecedor interno: alimente em 5 V e aguarde cerca de 2 min antes de confiar
> na leitura. Na demonstração, gás de isqueiro sem acender (MQ-2) ou álcool
> (MQ-135) faz a leitura subir rapidamente.

### Se alguma máquina for montada na outra placa

| Sinal | ESP32 DevKit (M01, M03) | ESP32-S3-N16R8 (M02) | LOLIN S2 Mini (opcional) |
|---|---|---|---|
| DHT22 DATA | GPIO 4 | GPIO 4 | GPIO 7 |
| Vazão (pulsos) | GPIO 33 | GPIO 5 | GPIO 5 |
| MQ A0 | GPIO 34 | GPIO 6 | GPIO 3 |
| LED RGB R / G / B | GPIO 25 / 26 / 27 | GPIO 15 / 16 / 17 | GPIO 9 / 11 / 12 |
| LED flash | GPIO 25 | GPIO 15 | GPIO 9 |
| 5 V para MQ e vazão | VIN | 5V | VBUS |

No S2 Mini só a fileira externa de pinos tem header
(`EN 3 5 7 9 11 12 3V3` / `39 37 35 33 18 16 GND VBUS`); os pinos acima usam
só ela.

### Divisores de tensão (obrigatórios em todo sensor alimentado em 5 V)

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

> **Saída do sensor de vazão**: é open collector, ou seja, só puxa para o GND e
> depende de um resistor de pull-up. Meça o fio de sinal antes de ligar no GPIO
> para saber se o seu modelo já tem pull-up interno para 5 V (aí vale o divisor)
> ou não (aí vale um pull-up de 10 kΩ para o 3V3, sem divisor). O procedimento
> está em [`../docs/10-esquema-eletrico.md`](../docs/10-esquema-eletrico.md).
> Alternativa sem divisor (padrão da M02): alimentar o sensor em **3V3** e
> compilar com `-D VAZAO_PULLUP_INTERNO=1` (o ESP32 fornece o pull-up). O
> YF-S402 é especificado a partir de 3,5 V e o YF-S201C a partir de 4,5 V; o
> YF-S402 pulsou em 3,3 V na bancada (28,6 Hz soprando). Para a M01, teste
> soprando antes de adotar.

> **Vazão em bancada seca**: sem água a leitura fica em 0 L/min (comportamento
> correto de bomba a seco); soprar na turbina gera vazão para a demonstração.
> Conversão por modelo, definida no `platformio.ini`
> (`FATOR_VAZAO_HZ_POR_LMIN`): YF-S201C = 7,5 Hz por L/min (M01);
> YF-S402 = 73 Hz por L/min (M02). Valores nominais (±10 %); para calibrar,
> compare o volume acumulado com um recipiente graduado.

## Comportamento

- Publica JSON a cada 10 s em `fabrica/maquinas/<ID>/telemetria`;
- Sinais sem sensor físico são simulados (variação normal + anomalia periódica
  a cada 15 min, Desafio 2);
- Status calculado no edge (0/1/2) aciona LED RGB (M01) e alarme (M03);
- O LED azul da própria placa dá uma piscada a cada publicação ("placa viva");
- Vazão real (M01/M02) é publicada sempre, mas só entra no status com
  `-D VAZAO_AFETA_STATUS=1` (evita alarme permanente em bancada seca);
- LWT: queda do dispositivo publica `offline` retido no tópico `status`;
- Timestamp via NTP (UTC-3); sem sincronismo, o back-end usa a hora de chegada;
- `TEMP_OFFSET` (padrão +40 °C) leva a leitura ambiente do DHT22 à faixa do
  cenário; aquecer o sensor com o dedo dispara o alerta ao vivo. Se a sala
  estiver muito quente ou muito fria no dia, ajuste com `-D TEMP_OFFSET=xx.0f`:
  a conta é `ambiente + offset` caindo entre 65 e 75 °C.
