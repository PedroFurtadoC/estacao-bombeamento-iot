# 10 - Esquema Elétrico

Ligação completa das três motobombas, pino a pino. Monte sempre na ordem deste
documento, do sensor mais seguro para o mais arriscado.

## O que ter em mãos antes de começar

| Item | Quantidade | Observação |
|---|---|---|
| LOLIN ESP32-S2 Mini | 1 | M01 (só a fileira externa de pinos tem header; **soldar a barra de pinos**) |
| ESP32-S3-N16R8 (formato DevKitC-1, 44 pinos) | 1 | M02 (header soldado; gravar pela porta `UART`/`COM`) |
| ESP32 DevKit (WROOM-32) | 1 (mais reserva) | M03 |
| Cabo USB de dados | 3 | cabo só de carga não grava a placa; S2 Mini e S3 usam USB-C |
| Protoboard | 3 pequenas ou 1 grande | uma montagem por máquina facilita |
| Jumpers macho-macho | cerca de 30 | ligação geral e conector do sensor de vazão |
| DHT22 / AM2302 | 3 | uma por placa |
| Sensor de vazão hall | 2 | YF-S201C (1/2") na M01 e YF-S402 (1/4") na M02 |
| MQ (gás) | 1 | M03 |
| HW-479 (LED RGB) | 1 | M01 |
| HW-481 (LED flash) | 1 | M03 |
| Resistor de 10 kΩ | 6 | divisores de tensão e pull-up |
| Resistor de 20 kΩ | 2 | divisores de tensão |
| Multímetro | 1 | recomendado, evita queimar placa |
| Carregador de 5 V, 1 A ou mais | 1 | para a M03, por causa do MQ |

Sobre os resistores: são **2 divisores obrigatórios**, um para a vazão da M01
e um para o MQ da M03. Cada divisor usa um resistor de 10 kΩ e um de 20 kΩ.
A vazão da M02 fica em 3,3 V e dispensa divisor (veja a seção da M02).

Se não encontrar os de 20 kΩ, dois de 10 kΩ em série fazem o mesmo papel, e aí
6 resistores de 10 kΩ resolvem tudo. Vale qualquer par na proporção de 1 para
2, como 1 kΩ com 2 kΩ.

Sem multímetro dá para montar, mas o documento indica em quais pontos ele
evita erro caro.

## Regras que valem para as três placas

**GND é comum.** Todo GND (da ESP32, dos sensores, dos divisores e dos LEDs)
precisa estar no mesmo trilho negativo da protoboard. Sem isso os sinais
flutuam e as leituras ficam sem sentido.

**3,3 V e 5 V não se misturam.** O DHT22 fica em 3,3 V e vai direto
no GPIO. A vazão da M01 e o MQ ficam em 5 V, e o sinal deles nunca pode
chegar cru no GPIO, porque o limite da ESP32 é 3,3 V. Todo sinal que sai de um
componente alimentado em 5 V passa antes por um divisor. A vazão da M02 é a
exceção: alimentada em 3,3 V, o sinal já nasce em 3,3 V e vai direto.

**De onde sai cada tensão na placa:**

| Pino da placa | Fornece | Alimenta |
|---|---|---|
| `3V3` | 3,3 V | DHT22 · vazão da M02 |
| `VIN` na DevKit (em algumas vem escrito `5V`) · `VBUS` no S2 Mini · `5V` no S3 | 5 V vindos do USB | Vazão da M01, MQ |
| `GND` (DevKit e S3 têm vários; o S2 Mini tem um na fileira externa) | referência comum | tudo |

## Placas desta montagem

O grupo tem **um LOLIN ESP32-S2 Mini, um ESP32-S3-N16R8 (formato DevKitC-1)
e uma ESP32 DevKit (WROOM-32)**. Cada
seção seguinte traz o esquema elétrico da máquina desenhado na placa que ela
usa; a tabela abaixo diz qual placa monta cada máquina e qual pino usar em
cada uma. O firmware escolhe o mapa de pinos sozinho pelo `board` do
environment (`firmware/platformio.ini`), então basta gravar o environment
certo na placa certa.

| Máquina | Placa | Sensores | Atuador |
|---|---|---|---|
| M01 | S2 Mini | DHT22 + vazão YF-S201C | LED RGB HW-479 |
| M02 | ESP32-S3-N16R8 | DHT22 + vazão YF-S402 | nenhum |
| M03 | DevKit | DHT22 + MQ (gás) | LED flash HW-481 |

**No S2 Mini só a fileira externa de pinos tem header.** Olhando a placa com
o USB-C para baixo, ela é:

```
esquerda : EN   3   5   7   9   11   12   3V3
direita  : 39   37  35  33  18  16   GND  VBUS
```

Os pinos da fileira interna (1, 2, 4, 6, 8, 10, 13, 14, 15, 17, 21, 34, 36,
38, 40) não são usados. `VBUS` é o 5 V do USB, equivalente ao `VIN` da DevKit.

| Sinal | S2 Mini (M01) | S3-N16R8 (M02) | DevKit (M03) |
|---|---|---|---|
| DHT22 DATA | GPIO 7 | GPIO 4 | GPIO 4 |
| Vazão (pulsos) | GPIO 5 | GPIO 5 | GPIO 33 |
| MQ A0 | GPIO 3 (ADC1) | GPIO 6 (ADC1) | GPIO 34 (ADC1) |
| LED RGB HW-479 R / G / B | GPIO 9 / 11 / 12 | GPIO 15 / 16 / 17 | GPIO 25 / 26 / 27 |
| LED flash HW-481 S | GPIO 9 | GPIO 15 | GPIO 25 |
| 3,3 V | `3V3` | `3V3` | `3V3` |
| 5 V (MQ e vazão) | `VBUS` | `5V` | `VIN` |

**No S3-N16R8 (44 pinos, todos escritos na serigrafia)** não use: `35`, `36`,
`37` (PSRAM octal do R8), `19`, `20` (USB), `43`, `44` (serial do monitor),
`0`, `3`, `45`, `46` (boot) e `48` (LED RGB da placa). Grave e monitore pela
porta USB-C marcada `UART`/`COM`.

Regras de tensão, divisores e pull-up valem igual nas três placas: o limite
de 3,3 V no GPIO é o mesmo. Nesta montagem o único analógico é o MQ, na DevKit
(GPIO 34, ADC1); se um MQ for parar num S2 Mini, use o GPIO 3, que é
ADC1 (o ADC2 do S2 para de funcionar com o Wi-Fi ligado).

**Duas figuras por máquina.** O esquema elétrico (o que liga em quê, com os
divisores e os símbolos de alimentação) está em cada seção abaixo e nos
arquivos `assets/esquema-m01.svg`, `-m02.svg` e `-m03.svg`. A **montagem furo
a furo na protoboard**, com a posição de cada módulo, jumper e resistor, está
em [`assets/protoboard.html`](assets/protoboard.html) (abra no navegador; as
figuras avulsas estão em `assets/protoboard-m01.svg`, `-m02.svg` e
`-m03.svg`). Todas prontas para o relatório. Dois pontos que essas figuras resolvem: no S2 Mini sobra
**um único furo livre por pino** (fileira `a` em cima e `j` embaixo), então
divisores e junções ficam nas colunas livres ao lado; e a DevKit de 30 pinos
tem 1,0" entre as fileiras e não atravessa uma protoboard comum: a fileira
com `3V3`, `GND`, `GPIO 4` e `GPIO 25` entra na protoboard e a outra fica no
ar, com um jumper fêmea-macho só no `GPIO 34` (e no `VIN`).

Como a serial do S2 Mini sai pela USB nativa, na **primeira gravação** é
preciso colocá-lo em modo de gravação na mão: com o USB desligado, segure o
botão `0`, ligue o USB e solte o `0` (ou segure `0`, toque `RST`, solte `0`).
Ele aparece como "Dispositivo Serial USB (COMx)"; mande o upload. Ao
terminar, a placa reinicia sozinha já com o firmware e a serial USB dele.
Nas próximas gravações o PlatformIO reinicia a placa sozinho pela serial USB,
sem botão. (O firmware traz um wrapper do esptool, `firmware/tools/`, que
trata o erro de porta que o esptool dá ao reiniciar o S2 pela USB nativa; sem
ele a gravação aparecia como `FAILED` mesmo tendo dado certo.)

**Trilhos da protoboard.** Antes de ligar qualquer sensor, puxe três fios da
ESP32 para os trilhos laterais: `3V3` para o trilho vermelho de cima, `VIN`
para o trilho vermelho de baixo e `GND` para os dois trilhos azuis, ligando os
dois entre si. Assim cada sensor busca a tensão certa no trilho certo.

**Como ler os esquemas.** As bandeiras `3V3` e `5 V` no alto e os símbolos
de terra embaixo são os nós de alimentação: tudo que aponta para a mesma
bandeira está ligado no mesmo pino da placa (`3V3`, `VBUS`/`VIN` ou `GND`).
Fio colorido é sinal, e a cor segue a legenda da figura. O ponto preto é uma
junção; `R1`/`R2` são o divisor de tensão. Pino tracejado com "não ligar"
fica solto.

## Máquina 01: DHT22, vazão e LED RGB

Placa: **S2 Mini**, sensor de vazão **YF-S201C**. Na tabela, o pino do S2
Mini vem primeiro e o da DevKit entre parênteses, caso a máquina seja montada
na outra placa.

![Esquema elétrico da M01: S2 Mini com DHT22, vazão YF-S201C via divisor e LED RGB HW-479](assets/esquema-m01.svg)

| Componente | Pino do componente | Vai para | Observação |
|---|---|---|---|
| DHT22 | VCC | 3V3 | **nunca no 5 V**, veja o aviso abaixo |
| DHT22 | DATA | GPIO 7 (DevKit: GPIO 4) | ligação direta, o módulo vermelho já tem pull-up interno |
| DHT22 | GND | GND | |
| Vazão YF-S201C | VCC (vermelho) | VBUS (DevKit: VIN), 5 V | |
| Vazão YF-S201C | GND (preto) | GND | |
| Vazão YF-S201C | SINAL (amarelo) | GPIO 5 (DevKit: GPIO 33) | **nunca direto**, veja a seção do divisor |
| HW-479 | R | GPIO 9 (DevKit: GPIO 25) | resistores já vêm na placa do módulo |
| HW-479 | G | GPIO 11 (DevKit: GPIO 26) | |
| HW-479 | B | GPIO 12 (DevKit: GPIO 27) | |
| HW-479 | GND | GND | é o pino marcado com um traço ou com o sinal de menos |

> **Por que o DHT22 fica no 3,3 V.** O datasheet do AM2302 aceita de 3,3 V a
> 5,5 V, mas a linha de dados sai na mesma tensão da alimentação. Ligando o
> sensor no 5 V, o DATA passaria a mandar 5 V no GPIO 4 e danificaria a placa.
> Em 3,3 V o sinal fica em 3,3 V, que é o correto para a ESP32.

## Máquina 02: DHT22 e vazão

Placa: **ESP32-S3-N16R8** (formato DevKitC-1), sensor de vazão **YF-S402**
(1/4"). É a M01 sem o LED, em outra placa e com a vazão **em 3,3 V, sem
divisor**: o sensor é alimentado pelo `3V3`, o sinal já sai em 3,3 V e entra
direto no GPIO 5; o pull-up que a saída open collector precisa vem da própria
ESP32 (`-D VAZAO_PULLUP_INTERNO=1`, já no environment `maquina02`). Validado
na bancada: soprando na turbina, 28,6 Hz. O S3 vem com header soldado e grava
pela porta `UART` sem botão.

![Esquema elétrico da M02: ESP32-S3 com DHT22 e vazão YF-S402 em 3,3 V, sem divisor](assets/esquema-m02.svg)

| Componente | Pino do componente | Vai para | Observação |
|---|---|---|---|
| DHT22 | VCC / DATA / GND | 3V3 / GPIO 4 / GND | `3V3` é o 1º pino da coluna esquerda; `4` fica logo abaixo do `RST` |
| Vazão YF-S402 | VCC (vermelho) | 3V3 | o S3 tem dois pinos `3V3`, um para cada sensor |
| Vazão YF-S402 | GND (preto) | GND | |
| Vazão YF-S402 | SINAL (amarelo) | GPIO 5 | **direto, sem resistor** (logo abaixo do `4`) |

O YF-S402 é especificado a partir de 3,5 V, mas pulsou normalmente em 3,3 V
no teste. Se algum dia não pulsar, a alternativa é a ligação da M01: vermelho
no `5V`, divisor 10k/20k no amarelo e a flag `VAZAO_PULLUP_INTERNO` removida.

O YF-S402 mede de 0,3 a 6 L/min (73 Hz por L/min); o YF-S201C da M01, de 1 a
30 L/min (7,5 Hz por L/min). Os dois pulsam soprando na turbina.

## Máquina 03: DHT22, MQ e LED flash

Placa: **ESP32 DevKit** (a única desta montagem), pinos exatamente como no
esquema. É a máquina que mais consome, por causa do aquecedor do MQ.

![Esquema elétrico da M03: DevKit com DHT22, MQ via divisor e LED flash HW-481](assets/esquema-m03.svg)

| Componente | Pino do componente | Vai para | Observação |
|---|---|---|---|
| DHT22 | VCC / DATA / GND | 3V3 / GPIO 4 / GND | igual às outras |
| MQ | VCC | VIN (5 V) | precisa de 5 V para o aquecedor funcionar |
| MQ | GND | GND | |
| MQ | A0 | GPIO 34 | **com divisor**, a saída chega perto de 5 V com gás |
| MQ | D0 | não usar | |
| HW-481 | S | GPIO 25 | o datasheet confirma acionamento direto por pino digital |
| HW-481 | pino do meio | não ligar | é N.C., não tem função |
| HW-481 | GND | GND | pino marcado com traço |

O MQ leva uns 2 minutos aquecendo antes de dar leitura confiável, e consome
cerca de 150 mA. Alimente a M03 por uma porta USB boa do notebook ou por um
carregador de 5 V com 1 A ou mais, senão a placa reinicia nos picos de Wi-Fi.

## O divisor de tensão

A conta é simples: o divisor entrega ao GPIO a fração `R2 / (R1 + R2)` da
tensão de entrada. Com 10 kΩ e 20 kΩ, os 5 V viram 3,33 V, que é exatamente o
limite da ESP32.

```
  Saída do sensor (até 5 V)
            |
          [ R1 = 10 kΩ ]
            |
            +-------------------> GPIO da ESP32
            |
          [ R2 = 20 kΩ ]
            |
           GND
```

Serve qualquer par na proporção de 1 para 2: 10 kΩ com 20 kΩ, 1 kΩ com 2 kΩ,
ou 10 kΩ com dois de 10 kΩ em série. Você precisa de **dois divisores**: um
para a vazão da M01 e um para o MQ da M03 (a vazão da M02 fica em 3,3 V).

Antes de plugar no GPIO, meça com multímetro a tensão entre o ponto do meio do
divisor e o GND. Tem que dar no máximo 3,3 V.

## Sensor de vazão: pull-up e adaptação dos fios

### O detalhe elétrico que muda a ligação

A saída do sensor hall é **open collector**, ou seja, o transistor interno só
consegue puxar o sinal para o GND. O nível alto quem define é o resistor de
pull-up. Alguns modelos já trazem esse resistor ligado ao VCC, outros não, e
isso muda o que você precisa montar.

**Descubra qual é o seu caso antes de ligar no GPIO:**

1. Ligue só o VCC (5 V) e o GND do sensor. Deixe o fio de sinal solto, sem
   encostar na ESP32;
2. Meça com o multímetro a tensão entre o fio de sinal e o GND;
3. Decida:

| Leitura | Significado | O que montar |
|---|---|---|
| Perto de 5 V | tem pull-up interno para 5 V | **divisor 10 kΩ e 20 kΩ**, como descrito acima |
| Perto de 0 V ou instável | não tem pull-up | resistor de **10 kΩ do sinal para o 3V3** e sinal direto no GPIO, sem divisor |

Os dois caminhos funcionam com o firmware como está, porque ele configura o
pino como entrada simples e espera o pull-up vindo de fora.

**Se não tiver multímetro**, use a montagem que funciona nos dois casos:
divisor de 10 kΩ e 20 kΩ mais um resistor de 4,7 kΩ ligando o fio de sinal ao
5 V. Com pull-up interno ou sem ele, o GPIO recebe entre 2,9 V e 3,0 V no nível
alto, que a ESP32 lê como alto com folga e sem passar do limite.

### Ligando os fios na protoboard

O sensor vem com conector fêmea de 2,54 mm, então não precisa cortar nem soldar
nada. Espete jumpers macho-macho dentro do conector e leve a outra ponta para a
protoboard.

Identificação dos fios, que segue a convenção dos sensores hall de vazão:

| Cor | Função | Vai para |
|---|---|---|
| Vermelho | VCC, de 5 V a 18 V | VBUS do S2 Mini (M01) · 5V do S3 (M02) |
| Preto | GND | trilho de GND |
| Amarelo | sinal de pulsos | GPIO 5 (nas duas placas), passando pelo divisor ou pelo pull-up |

Fatores de conversão dos dois modelos (já definidos por máquina no
`firmware/platformio.ini`): YF-S201C, 7,5 Hz por L/min (450 pulsos por
litro, 1 a 30 L/min); YF-S402, 73 Hz por L/min (4380 pulsos por litro,
0,3 a 6 L/min). São valores nominais com ±10 % de tolerância.

> **Alternativa sem divisor (é o padrão da M02):** alimentar o sensor em
> `3V3` e compilar com `-D VAZAO_PULLUP_INTERNO=1`. O sinal passa a oscilar
> entre 0 e 3,3 V e vai direto no GPIO, com o pull-up fornecido pelo próprio
> ESP32. O YF-S402 é especificado a partir de 3,5 V e o YF-S201C a partir de
> 4,5 V; o YF-S402 da M02 pulsou em 3,3 V no teste (28,6 Hz soprando). Para o
> YF-S201C da M01, confirme soprando antes de adotar.

Se algum jumper ficar folgado no conector, aperte levemente o contato metálico
dele com um alicate antes de espetar de novo. Mau contato aqui aparece como
vazão travada em 0.

## Antes de energizar, confira

1. Nenhum fio de 5 V encostando em pino de 3,3 V;
2. Todos os GND no mesmo trilho, incluindo o dos divisores;
3. Os dois divisores (vazão da M01 e MQ da M03) medindo no máximo 3,3 V na saída;
4. DHT22 no 3V3, nunca no VIN;
5. MQ e vazão da M01 no 5 V (`VIN`/`VBUS`), nunca no 3V3, senão não funcionam
   direito; a vazão da M02 é a exceção, no `3V3` e sem divisor;
6. Na DevKit, nada ligado nos pinos `EN`, `GPIO 0`, `GPIO 2`, `GPIO 12` e
   `GPIO 15`, que interferem no boot da placa (no S2 Mini esses números não
   têm essa função; o GPIO 12 do S2 Mini é um pino comum e é usado pelo LED).
   No S3, os pinos de boot são `0`, `3`, `45` e `46`;
7. Cada placa etiquetada com M01, M02 ou M03, batendo com o firmware gravado.

## Teste pino a pino, depois de ligar

| O que testar | Como | Esperado |
|---|---|---|
| Placa viva | monitor serial a 115200 | mensagens de Wi-Fi e MQTT |
| DHT22 | olhar a linha `[pub]` | temperatura por volta de 70, por causa do offset |
| DHT22 reagindo | segurar o sensor entre os dedos | valor sobe em poucos segundos |
| MQ | isqueiro sem acender, após 2 min ligado | valor de gás sobe |
| Vazão | soprar na turbina | vazão sai de 0 |
| LED RGB | aquecer o DHT22 até passar de 80 | verde vira amarelo e depois vermelho |
| LED flash | aquecer o DHT22 da M03 ou gás no MQ acima de 40 % | acende piscando no crítico |

Roteiro do dia da apresentação e solução de problemas:
[`09-guia-de-bancada.md`](09-guia-de-bancada.md).

## Fontes consultadas

As decisões de tensão e de ligação deste documento vieram destas referências,
e não de suposição:

| Componente | O que foi confirmado | Fonte |
|---|---|---|
| DHT22 / AM2302 | Alimentação de 3,3 V a 5,5 V, e a linha de dados sai na mesma tensão da alimentação, por isso fica no 3,3 V | [Datasheet AM2302](https://www.edn.com/am2302-dht22-datasheet/) |
| Sensor de vazão YF-S201 | Saída hall é open collector e depende de resistor de pull-up; o sinal em 5 V precisa de divisor para a ESP32 | [Cirkit Designer](https://docs.cirkitdesigner.com/component/0c226b84-d050-7693-529d-dc7a3152bb1a/yf-s201-water-flow-meter) |
| HW-481 / KY-034 | Faixa de 3,3 V a 5 V, pinagem GND, N.C. e Signal, acionado direto por pino digital, com resistor de 100 Ω na própria placa | [Datasheet Joy-IT KY-034](https://sensorkit.joy-it.net/en/sensors/ky-034) |
| HW-479 / KY-016 | LED RGB catodo comum com resistores de 150 Ω embutidos, sem necessidade de resistor externo | [Referência KY-016](https://arduinomodules.info/ky-016-rgb-full-color-led-module/) |
| GPIO 33 da ESP32 | Pino válido e seguro para interrupção, usado aqui na contagem de pulsos | [ESP32 GPIO Interrupts](https://randomnerdtutorials.com/esp32-gpio-interrupts-arduino/) |
