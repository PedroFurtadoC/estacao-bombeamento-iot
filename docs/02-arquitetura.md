# 02 - Arquitetura (Desafio 1)

## Diagrama

```mermaid
flowchart LR
    subgraph Maquinas["Estação de bombeamento (3 conjuntos motobomba)"]
        direction TB
        subgraph M01["Motobomba 01"]
            S1["DHT22 + vazão hall<br/>temperatura + vazão"] --> E1["ESP32<br/>DevKit"]
            E1 --> A1["LED RGB HW-479<br/>semáforo de status"]
        end
        subgraph M02["Motobomba 02"]
            S2["DHT22 + vazão hall<br/>temperatura + vazão"] --> E2["ESP32-S3<br/>DevKitC-1"]
        end
        subgraph M03["Motobomba 03"]
            S3["DHT22 + MQ<br/>temperatura + gás"] --> E3["ESP32<br/>DevKit"]
            E3 --> A3["LED flash HW-481<br/>alarme crítico"]
        end
    end

    E1 & E2 & E3 -- "Wi-Fi (hotspot)<br/>MQTT / JSON" --> BR["Mosquitto<br/>broker MQTT<br/>(Docker)"]
    BR -- "subscribe<br/>fabrica/maquinas/+/telemetria" --> IN["Serviço de Ingestão<br/>Python + paho-mqtt<br/>(Docker)"]
    IN -- "line protocol<br/>API v2" --> DB[("InfluxDB 2<br/>bucket iot<br/>(Docker)")]
    DB -- "Flux" --> GF["Grafana<br/>dashboard provisionado<br/>(Docker)"]
    GF -- "HTTP :3000" --> US(("Usuário<br/>operador da fábrica"))
```

## Por que cada peça

### Sensores

O professor sugeriu um sensor ambiental por ESP32, tipo BME280. Usamos o DHT22
porque é o que o grupo tinha: temperatura e umidade em um fio só, com
biblioteca pronta. A umidade entrou de brinde, mas tem sentido no cenário —
umidade alta na casa de bombas costuma ser vazamento.

Na M01 e na M02 tem sensor de vazão de turbina com efeito hall, que gera pulsos
proporcionais à vazão. Para uma bomba, vazão é o principal indicador de
processo: se cai, tem obstrução ou cavitação; se zera com a bomba ligada, ela
está trabalhando a seco. Junto com a corrente do motor, fecha o par
causa/efeito.

A M03 tem um sensor de gás MQ no lugar da vazão. É um sinal de segurança, não
de processo: casa de bombas é espaço confinado, e acúmulo de gás ali é risco
real. O enunciado permitia escolher sinais extras, e esse é o que faz mais
sentido no nosso cenário.

Corrente, rotação e vibração não têm sensor físico em nenhuma das máquinas
(nem vazão na M03). Esses sinais são gerados no próprio ESP32, com variação
aleatória dentro das faixas do cenário e janelas de anomalia periódicas. O
enunciado permite simular, e é a mesma técnica de uma bancada de teste ou de um
gêmeo digital: o caminho do dado é idêntico, só a origem muda. Quem lê o
dashboard vê no campo `fonte` se a leitura é `hibrido` (tem sensor real no
meio) ou `simulado`.

### ESP32, uma por máquina

Wi-Fi integrado, ADC de 12 bits, preço baixo e o ecossistema
Arduino/PlatformIO, que o grupo já conhecia. Uma placa por máquina, e não uma
placa lendo três máquinas, porque é assim numa fábrica de verdade: cada ativo
tem seu nó de aquisição, e a queda de um não derruba os outros.

A placa também decide sozinha se a leitura é normal, de atenção ou crítica, e
acende o LED de acordo. Isso é o processamento de borda do Desafio 4: a
resposta local sai em milissegundos e continua funcionando com a rede caída.

Na prática não são três placas iguais. A M02 acabou num ESP32-S3 porque as
LOLIN S2 Mini que tínhamos vieram com a barra de pinos solta, e jumper
espetado no furo sem solda não faz contato — perdemos uma noite inteira nisso
antes de entender. O código é o mesmo para as três; só o mapa de pinos muda,
escolhido em tempo de compilação pelo alvo (`firmware/src/config.h`).

### MQTT com Mosquitto

MQTT é o protocolo padrão em IoT e se encaixa bem aqui: o cabeçalho é pequeno,
o que importa num microcontrolador, e o modelo publish/subscribe desacopla as
placas do back-end. A ESP32 publica sem saber quem está do outro lado; podemos
derrubar e subir a ingestão sem tocar no firmware.

Também usamos duas coisas que o protocolo dá de graça: o LWT (last will), que
faz o broker anunciar `offline` sozinho quando uma placa some, e mensagens
retidas no tópico de status, para quem conectar depois saber o estado atual.

Broker é o Eclipse Mosquitto, que é leve, open source e sobe em um contêiner.
A rede é o hotspot do notebook, como o professor recomendou, para não depender
do Wi-Fi da universidade.

### Ingestão em Python

Um serviço próprio, em vez de ligar o broker direto no banco com Telegraf. Fica
mais código para manter, mas é justamente a camada de back-end que o enunciado
pede, e dá um lugar para validar: JSON malformado, máquina desconhecida, campo
que veio como texto — tudo isso é descartado com log em vez de virar lixo no
banco. É também onde o status é recalculado quando o payload não traz o dele.

### InfluxDB

Banco de série temporal, que é exatamente o formato do dado: um valor, um
instante, algumas etiquetas. Ganhamos consulta por janela de tempo, retenção
automática (30 dias no bucket) e integração direta com o Grafana. Um Postgres
daria conta, mas teríamos que escrever à mão o que o Influx já faz.

### Grafana

Citado no enunciado e é o que se usa no mercado. O que mais pesou na prática:
dashboard, datasource e alertas ficam em arquivo dentro do repositório e sobem
junto com a stack. Qualquer um do grupo levanta o projeto e vê exatamente a
mesma tela, sem ninguém precisar reconstruir painel na mão.

### Usuário

Quem consome é o operador da estação: acompanha os sinais, percebe tendência
(aquecimento gradual, vazão caindo) e reage aos alertas.

## O caminho do dado

1. O sensor é lido, ou o firmware gera o sinal simulado;
2. a ESP32 monta o JSON, classifica o status e publica via MQTT;
3. o Mosquitto entrega a quem estiver assinando;
4. a ingestão valida e grava no InfluxDB, preservando o timestamp da placa;
5. o Grafana consulta e desenha;
6. o operador olha e decide.

## Decisões e alternativas

| Decisão | Alternativa | Por que ficamos com a escolhida |
|---|---|---|
| MQTT | HTTP REST | Menos overhead por mensagem e pub/sub desacoplado, que é o padrão em IoT |
| InfluxDB | PostgreSQL | O dado é série temporal; retenção e janela de tempo vêm prontas |
| Serviço Python próprio | Telegraf / Node-RED | É a camada de back-end que o enunciado pede, e dá onde validar o payload |
| Docker Compose | Instalar cada serviço na mão | Um comando levanta tudo igual em qualquer notebook do grupo |
| Tudo local | Hospedar na nuvem | A demo precisa rodar no hotspot, sem internet e sem custo; o mesmo código subiria para um servidor trocando o endereço do broker |
