# 10 - Esquema Elétrico

Ligação completa das três motobombas, pino a pino. Monte sempre na ordem deste
documento, do sensor mais seguro para o mais arriscado.

## O que ter em mãos antes de começar

| Item | Quantidade | Observação |
|---|---|---|
| ESP32 DevKit (WROOM-32, 30 pinos) | 2 (mais reserva) | M01 e M03 (header soldado; gravar pela micro-USB) |
| ESP32-S3-N16R8 (formato DevKitC-1, 44 pinos) | 1 | M02 (header soldado; gravar pela porta `UART`/`COM`) |
| Cabo USB de dados | 3 | cabo só de carga não grava a placa; DevKit usa micro-USB, S3 usa USB-C |
| Protoboard | 3 pequenas ou 1 grande | uma montagem por máquina facilita |
| Jumpers macho-macho | cerca de 30 | ligação geral e conector do sensor de vazão |
| Jumpers fêmea-macho | 4 | a fileira da DevKit que fica fora da protoboard (2 por DevKit) |
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
| `VIN` na DevKit (em algumas vem escrito `5V`) · `5V` no S3 | 5 V vindos do USB | Vazão da M01, MQ |
| `GND` (DevKit e S3 têm vários) | referência comum | tudo |

## Placas desta montagem

O grupo tem **duas ESP32 DevKit (WROOM-32, 30 pinos) e um ESP32-S3-N16R8
(formato DevKitC-1)**. Cada seção seguinte traz o esquema elétrico da máquina
desenhado na placa que ela usa; a tabela abaixo diz qual placa monta cada
máquina e qual pino usar em cada uma. O firmware escolhe o mapa de pinos
sozinho pelo `board` do environment (`firmware/platformio.ini`), então basta
gravar o environment certo na placa certa.

| Máquina | Placa | Sensores | Atuador |
|---|---|---|---|
| M01 | DevKit | DHT22 + vazão YF-S201C | LED RGB HW-479 |
| M02 | ESP32-S3-N16R8 | DHT22 + vazão YF-S402 | nenhum |
| M03 | DevKit | DHT22 + MQ (gás) | LED flash HW-481 |

**A DevKit de 30 pinos** tem duas fileiras de 15 pinos, escritas na
serigrafia. Olhando a placa com o USB para a esquerda:

```
fileira de cima : 3V3  GND  15  2   4   16  17  5   18  19  21  RX0  TX0  22  23
fileira de baixo: VIN  GND  13  12  14  27  26  25  33  32  35  34   VN   VP  EN
```

Tudo que a M01 e a M03 usam está nessas duas fileiras: `3V3` e `GPIO 4`
(DHT22) numa, `VIN`, `GND`, os `GPIO 25/26/27` (LED), `33` (vazão) e `34`
(MQ) na outra. Não ligue nada em `EN`, `0`, `2`, `12` e `15` (boot) nem nos
pinos `6` a `11` (não vêm no header: são a flash interna). `34`, `35`, `VP` e
`VN` são somente entrada.

| Sinal | DevKit (M01 e M03) | S3-N16R8 (M02) |
|---|---|---|
| DHT22 DATA | GPIO 4 | GPIO 4 |
| Vazão (pulsos) | GPIO 33 | GPIO 5 |
| MQ A0 | GPIO 34 (ADC1) | GPIO 6 (ADC1) |
| LED RGB HW-479 R / G / B | GPIO 25 / 26 / 27 | GPIO 15 / 16 / 17 |
| LED flash HW-481 S | GPIO 25 | GPIO 15 |
| 3,3 V | `3V3` | `3V3` |
| 5 V (MQ e vazão) | `VIN` | `5V` |

(Se alguma máquina for montada num LOLIN S2 Mini, o firmware também o
suporta; os pinos dele estão em `firmware/README.md`.)

**No S3-N16R8 (44 pinos, todos escritos na serigrafia)** não use: `35`, `36`,
`37` (PSRAM octal do R8), `19`, `20` (USB), `43`, `44` (serial do monitor),
`0`, `3`, `45`, `46` (boot) e `48` (LED RGB da placa). Grave e monitore pela
porta USB-C marcada `UART`/`COM`.

Regras de tensão, divisores e pull-up valem igual nas três placas: o limite
de 3,3 V no GPIO é o mesmo. Nesta montagem o único analógico é o MQ, na DevKit
da M03 (GPIO 34, ADC1; o ADC2 para de funcionar com o Wi-Fi ligado).

**Duas figuras por máquina.** O esquema elétrico (o que liga em quê, com os
divisores e os símbolos de alimentação) está em cada seção abaixo e nos
arquivos `assets/esquema-m01.svg`, `-m02.svg` e `-m03.svg`. A **montagem furo
a furo na protoboard**, com a posição de cada módulo, jumper e resistor, está
em [`assets/protoboard.html`](assets/protoboard.html) (abra no navegador; as
figuras avulsas estão em `assets/protoboard-m01.svg`, `-m02.svg` e
`-m03.svg`). Todas prontas para o relatório. O ponto que essas figuras
resolvem: a DevKit de 30 pinos tem 1,0" entre as fileiras e não atravessa uma
protoboard comum. A fileira de baixo (`VIN`, `GND`, `25/26/27`, `33`, `34`)
entra na fileira `a` da protoboard e a de cima fica no ar, com **dois jumpers
fêmea-macho** por DevKit: `3V3` para o trilho vermelho e `GPIO 4` para o DATA
do DHT22. Vale para a M01 e para a M03, que usam exatamente a mesma posição.

A DevKit grava pela micro-USB sem preparo. Se o upload travar em
`Connecting......`, segure o botão `BOOT` da placa até começar a gravar (em
algumas DevKit o auto-reset do chip USB-serial não funciona).

**Trilhos da protoboard.** Antes de ligar qualquer sensor, puxe três fios da
ESP32 para os trilhos laterais: `3V3` para o trilho vermelho de cima, `VIN`
para o trilho vermelho de baixo e `GND` para os dois trilhos azuis, ligando os
dois entre si. Assim cada sensor busca a tensão certa no trilho certo.

**Como ler os esquemas.** As bandeiras `3V3` e `5 V` no alto e os símbolos
de terra embaixo são os nós de alimentação: tudo que aponta para a mesma
bandeira está ligado no mesmo pino da placa (`3V3`, `VIN`/`5V` ou `GND`).
Fio colorido é sinal, e a cor segue a legenda da figura. O ponto preto é uma
junção; `R1`/`R2` são o divisor de tensão. Pino tracejado com "não ligar"
fica solto.

## Máquina 01: DHT22, vazão e LED RGB

Placa: **ESP32 DevKit** (WROOM-32), sensor de vazão **YF-S201C**. Todos os
pinos usados, menos `3V3` e `GPIO 4`, ficam na fileira do `VIN`; os dois da
outra fileira chegam por jumper fêmea-macho (veja a protoboard).

![Esquema elétrico da M01: ESP32 DevKit com DHT22, vazão YF-S201C via divisor e LED RGB HW-479](assets/esquema-m01.svg)

| Componente | Pino do componente | Vai para | Observação |
|---|---|---|---|
| DHT22 | VCC | 3V3 | **nunca no 5 V**, veja o aviso abaixo |
| DHT22 | DATA | GPIO 4 | ligação direta, o módulo vermelho já tem pull-up interno |
| DHT22 | GND | GND | |
| Vazão YF-S201C | VCC (vermelho) | VIN, 5 V | |
| Vazão YF-S201C | GND (preto) | GND | |
| Vazão YF-S201C | SINAL (amarelo) | GPIO 33 | **nunca direto**, veja a seção do divisor |
| HW-479 | R | GPIO 25 | resistores já vêm na placa do módulo |
| HW-479 | G | GPIO 26 | |
| HW-479 | B | GPIO 27 | |
| HW-479 | GND | GND | é o pino marcado com um traço ou com o sinal de menos |

Os `GPIO 25`, `26` e `27` são vizinhos na fileira do `VIN`, na mesma ordem
`27 26 25`; o `33` vem logo depois do `25`. Isso deixa o LED e a vazão lado a
lado na protoboard, como mostra a figura.

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

Placa: **ESP32 DevKit** (igual à da M01), pinos exatamente como no
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
| Vermelho | VCC, de 5 V a 18 V | VIN da DevKit (M01) · 3V3 do S3 (M02, ver alternativa abaixo) |
| Preto | GND | trilho de GND |
| Amarelo | sinal de pulsos | GPIO 33 na M01 (pelo divisor) · GPIO 5 na M02 (direto) |

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
5. MQ e vazão da M01 no 5 V (`VIN`), nunca no 3V3, senão não funcionam
   direito; a vazão da M02 é a exceção, no `3V3` e sem divisor;
6. Nas DevKit (M01 e M03), nada ligado nos pinos `EN`, `GPIO 0`, `GPIO 2`,
   `GPIO 12` e `GPIO 15`, que interferem no boot da placa. No S3, os pinos de
   boot são `0`, `3`, `45` e `46`;
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
