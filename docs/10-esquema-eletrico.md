# 10 - Esquema Elétrico

Ligação completa das três motobombas, pino a pino. Monte sempre na ordem deste
documento, do sensor mais seguro para o mais arriscado.

## Regras que valem para as três placas

**GND é comum.** Todo GND (da ESP32, dos sensores, dos divisores e dos LEDs)
precisa estar no mesmo trilho negativo da protoboard. Sem isso os sinais
flutuam e as leituras ficam sem sentido.

**3,3 V e 5 V não se misturam.** O DHT22 e o HW-484 ficam em 3,3 V e vão direto
no GPIO. O sensor de vazão e o MQ ficam em 5 V, e o sinal deles nunca pode
chegar cru no GPIO, porque o limite da ESP32 é 3,3 V. Todo sinal que sai de um
componente alimentado em 5 V passa antes por um divisor.

**De onde sai cada tensão na ESP32 DevKit:**

| Pino da placa | Fornece | Alimenta |
|---|---|---|
| `3V3` | 3,3 V | DHT22, HW-484 |
| `VIN` (em algumas placas vem escrito `5V`) | 5 V vindos do USB | Sensor de vazão, MQ |
| `GND` (a placa tem vários) | referência comum | tudo |

**Trilhos da protoboard.** Antes de ligar qualquer sensor, puxe três fios da
ESP32 para os trilhos laterais: `3V3` para o trilho vermelho de cima, `VIN`
para o trilho vermelho de baixo e `GND` para os dois trilhos azuis, ligando os
dois entre si. Assim cada sensor busca a tensão certa no trilho certo.

## Máquina 01: DHT22, vazão e LED RGB

```
   ESP32 DevKit                          Componentes
   +-----------+
   |      3V3  o--------------------o VCC   DHT22 (AM2302)
   |    GPIO 4 o--------------------o DATA
   |      GND  o--------------------o GND
   |           |
   |      VIN  o--------------------o VCC (vermelho)  sensor de vazão
   |      GND  o--------------------o GND (preto)
   |   GPIO 33 o------[ divisor ]---o SINAL (amarelo)
   |           |
   |   GPIO 25 o--------------------o R    LED RGB HW-479
   |   GPIO 26 o--------------------o G
   |   GPIO 27 o--------------------o B
   |      GND  o--------------------o GND (pino marcado com traço)
   +-----------+
```

| Componente | Pino do componente | Vai para | Observação |
|---|---|---|---|
| DHT22 | VCC | 3V3 | **nunca no 5 V**, veja o aviso abaixo |
| DHT22 | DATA | GPIO 4 | ligação direta, o módulo vermelho já tem pull-up interno |
| DHT22 | GND | GND | |
| Vazão | VCC (vermelho) | VIN (5 V) | |
| Vazão | GND (preto) | GND | |
| Vazão | SINAL (amarelo) | GPIO 33 | **nunca direto**, veja a seção do divisor |
| HW-479 | R | GPIO 25 | resistores já vêm na placa do módulo |
| HW-479 | G | GPIO 26 | |
| HW-479 | B | GPIO 27 | |
| HW-479 | GND | GND | é o pino marcado com um traço ou com o sinal de menos |

> **Por que o DHT22 fica no 3,3 V.** O datasheet do AM2302 aceita de 3,3 V a
> 5,5 V, mas a linha de dados sai na mesma tensão da alimentação. Ligando o
> sensor no 5 V, o DATA passaria a mandar 5 V no GPIO 4 e danificaria a placa.
> Em 3,3 V o sinal fica em 3,3 V, que é o correto para a ESP32.

## Máquina 02: DHT22 e HW-484

A mais simples das três. Na versão de 3,3 V do HW-484, que é a mais comum, não
precisa de divisor nenhum. Leia o aviso das duas versões no fim desta seção.

```
   ESP32 DevKit                          Componentes
   +-----------+
   |      3V3  o----+---------------o VCC   DHT22 (AM2302)
   |    GPIO 4 o----|---------------o DATA
   |      GND  o----|---+-----------o GND
   |           |    |   |
   |           |    +---|-----------o +     HW-484 (microfone)
   |      GND  o--------+-----------o G
   |   GPIO 34 o--------------------o A0
   +-----------+
```

| Componente | Pino do componente | Vai para | Observação |
|---|---|---|---|
| DHT22 | VCC / DATA / GND | 3V3 / GPIO 4 / GND | igual à M01 |
| HW-484 | + | 3V3 | |
| HW-484 | G | GND | |
| HW-484 | A0 | GPIO 34 | use a saída **analógica**, não a D0 |

O HW-484 tem um trimpot (o parafuso azul). Ele regula o limiar da saída
digital D0, que não usamos, mas girando no sentido horário a sensibilidade
aumenta. Se a vibração não reagir quando você bater na bancada, ajuste ele
primeiro.

> **Atenção, o HW-484 tem duas versões no mercado.** Uma trabalha de 3,3 V a
> 5 V e outra exige de 4 V a 6 V. Como não dá para saber pela aparência, comece
> sempre em 3,3 V, que é a ligação segura. Bata perto do microfone e veja se a
> vibração responde no serial. Se não responder nem mexendo no trimpot, o seu
> módulo é da versão de 4 V a 6 V: passe o VCC para o VIN e **coloque um divisor
> de 10 kΩ e 20 kΩ na saída A0**, porque em 5 V ela ultrapassa os 3,3 V que o
> GPIO aguenta. Nunca ligue o A0 direto no GPIO com o módulo em 5 V.

## Máquina 03: DHT22, MQ, vazão e LED flash

A placa mais cheia e a única com dois sensores de 5 V.

```
   ESP32 DevKit                          Componentes
   +-----------+
   |      3V3  o--------------------o VCC   DHT22 (AM2302)
   |    GPIO 4 o--------------------o DATA
   |      GND  o--------------------o GND
   |           |
   |      VIN  o----+---------------o VCC   MQ (gás)
   |      GND  o----|---+-----------o GND
   |   GPIO 34 o----|---|--[ div ]--o A0
   |           |    |   |
   |      VIN  o----+---|-----------o VCC (vermelho)  sensor de vazão
   |      GND  o--------+-----------o GND (preto)
   |   GPIO 33 o-----------[ div ]--o SINAL (amarelo)
   |           |
   |   GPIO 25 o--------------------o S     LED flash HW-481
   |      GND  o--------------------o GND
   +-----------+
```

| Componente | Pino do componente | Vai para | Observação |
|---|---|---|---|
| DHT22 | VCC / DATA / GND | 3V3 / GPIO 4 / GND | igual às outras |
| MQ | VCC | VIN (5 V) | precisa de 5 V para o aquecedor funcionar |
| MQ | GND | GND | |
| MQ | A0 | GPIO 34 | **com divisor**, a saída chega perto de 5 V com gás |
| MQ | D0 | não usar | |
| Vazão | VCC / GND / SINAL | VIN / GND / GPIO 33 | **com divisor** |
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
ou 10 kΩ com dois de 10 kΩ em série. Você precisa de **dois divisores**, um
para o MQ e outro para o sensor de vazão da M03, mais um para a vazão da M01.
No total são três.

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

### Como levar os fios para a protoboard

Fio flexível não segura na protoboard e ainda pode encostar no furo vizinho.
Não corte o conector por impulso, ele é a parte mais fácil de usar. Em ordem de
preferência:

1. **O sensor veio com conector fêmea de 2,54 mm**, que é o caso mais comum dos
   kits para Arduino. Basta espetar jumpers macho-macho dentro do conector e
   levar a outra ponta para a protoboard. Zero modificação, e dá para desmontar
   depois;
2. **Soldar uma barra de pinos macho** nas pontas dos fios. É a solução mais
   firme e continua reversível, porque o sensor não é danificado;
3. **Estanhar as pontas**, se você cortar mesmo. Corte, desencape uns 5 mm,
   torça bem os filamentos e passe solda na ponta para virar um pino rígido.
   Sem estanhar, o fio esfarela dentro do furo e faz mau contato;
4. **Borne de parafuso** na protoboard, se tiverem um. Prende o fio flexível
   sem solda nenhuma.

Só corte se nenhuma das opções acima der, e tenha ferro de solda por perto.

### Identificando os fios

A convenção dos sensores hall de vazão é por cor:

| Cor | Função |
|---|---|
| Vermelho | VCC, de 5 V a 18 V |
| Preto | GND |
| Amarelo | sinal de pulsos |

Se o seu tiver um quarto fio ou um quarto pino no conector, ele costuma ser
blindagem ou não ter uso. Na dúvida, confirme com o multímetro em continuidade:
o preto tem continuidade com a carcaça metálica na maioria dos modelos.

## Antes de energizar, confira

1. Nenhum fio de 5 V encostando em pino de 3,3 V;
2. Todos os GND no mesmo trilho, incluindo o dos divisores;
3. Os três divisores medindo no máximo 3,3 V na saída;
4. DHT22 no 3V3, nunca no VIN;
5. MQ e vazão no VIN, nunca no 3V3, senão não funcionam direito;
6. Nada ligado nos pinos `EN`, `GPIO 0`, `GPIO 2`, `GPIO 12` e `GPIO 15`, que
   interferem no boot da placa;
7. Cada placa etiquetada com M01, M02 ou M03, batendo com o firmware gravado.

## Teste pino a pino, depois de ligar

| O que testar | Como | Esperado |
|---|---|---|
| Placa viva | monitor serial a 115200 | mensagens de Wi-Fi e MQTT |
| DHT22 | olhar a linha `[pub]` | temperatura por volta de 70, por causa do offset |
| DHT22 reagindo | segurar o sensor entre os dedos | valor sobe em poucos segundos |
| HW-484 | bater na bancada | pico na vibração |
| MQ | isqueiro sem acender, após 2 min ligado | valor de gás sobe |
| Vazão | soprar na turbina | vazão sai de 0 |
| LED RGB | aquecer o DHT22 até passar de 80 | verde vira amarelo e depois vermelho |
| LED flash | mesma coisa na M03 | acende piscando no crítico |

Roteiro do dia da apresentação e solução de problemas:
[`09-guia-de-bancada.md`](09-guia-de-bancada.md).

## Fontes consultadas

As decisões de tensão e de ligação deste documento vieram destas referências,
e não de suposição:

| Componente | O que foi confirmado | Fonte |
|---|---|---|
| DHT22 / AM2302 | Alimentação de 3,3 V a 5,5 V, e a linha de dados sai na mesma tensão da alimentação, por isso fica no 3,3 V | [Datasheet AM2302](https://www.edn.com/am2302-dht22-datasheet/) |
| Sensor de vazão YF-S201 | Saída hall é open collector e depende de resistor de pull-up; o sinal em 5 V precisa de divisor para a ESP32 | [Cirkit Designer](https://docs.cirkitdesigner.com/component/0c226b84-d050-7693-529d-dc7a3152bb1a/yf-s201-water-flow-meter) |
| HW-484 / KY-038 | Existem duas versões, uma de 3,3 V a 5 V e outra de 4 V a 6 V | [Notas sobre os módulos LM386, HW-484 e KY-038](https://neonaut.neocities.org/blog/2018/sound-sensor-modules-lm386-hw-484-and-ky-038) |
| HW-481 / KY-034 | Faixa de 3,3 V a 5 V, pinagem GND, N.C. e Signal, acionado direto por pino digital, com resistor de 100 Ω na própria placa | [Datasheet Joy-IT KY-034](https://sensorkit.joy-it.net/en/sensors/ky-034) |
| HW-479 / KY-016 | LED RGB catodo comum com resistores de 150 Ω embutidos, sem necessidade de resistor externo | [Referência KY-016](https://arduinomodules.info/ky-016-rgb-full-color-led-module/) |
| GPIO 33 da ESP32 | Pino válido e seguro para interrupção, usado aqui na contagem de pulsos | [ESP32 GPIO Interrupts](https://randomnerdtutorials.com/esp32-gpio-interrupts-arduino/) |
