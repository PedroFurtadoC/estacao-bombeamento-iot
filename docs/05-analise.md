# 05 - Análise (Desafio 4)

As perguntas 1 a 5 são respondidas com a base coletada em 23/09/2026: 2160
registros, 720 por máquina, exportados para `dados/export/telemetria.csv` pelo
`backend/tools/exportar_csv.py`. É um backfill de 2 h do simulador, com a
janela de anomalia na M01. Se a base for gerada de novo (por exemplo com as
ESP32 ligadas no dia da apresentação), os números mudam, e é só refazer pelos
mesmos painéis indicados em cada resposta. As perguntas 6 a 10 são sobre a
arquitetura e não dependem da base.

## 1. Qual máquina apresentou maior temperatura?

A M01, com pico de 83,3 °C em 23/09/2026 às 21:08:41. A M02 e a M03 não
passaram de 72,4 °C e 70,9 °C, ou seja, ficaram dentro da faixa normal o tempo
todo.

Onde ver: painel "Temperatura ao longo do tempo", olhando o máximo de cada
série; ou `max()` em Flux agrupado por `machine`.

## 2. Qual máquina apresentou maior vibração?

De novo a M01, com 5,8 mm/s às 21:06:01, acima dos 5 mm/s que definimos como
faixa crítica. M02 e M03 chegaram no máximo a 3,4 mm/s.

Mesmo procedimento, no painel de vibração.

## 3. Existe relação entre temperatura e corrente?

Existe, e é positiva. Nas 720 leituras da M01 o coeficiente de Pearson entre
temperatura e corrente deu r = 0,71. Separando as duas fases da mesma máquina
a relação fica mais clara:

| Fase (M01) | Leituras | Temperatura | Corrente | Rotação | Vibração | Vazão |
|---|---|---|---|---|---|---|
| Normal | 681 | 69,6 °C | 7,0 A | 3515 RPM | 1,96 mm/s | 32,9 L/min |
| Anomalia | 39 | 81,6 °C | 11,2 A | 3273 RPM | 5,30 mm/s | 13,3 L/min |

A explicação é direta: mais carga no motor puxa mais corrente, e a corrente
aquece o enrolamento por efeito Joule (I²R). Por isso as duas séries sobem
juntas enquanto a rotação cai: é a assinatura de sobrecarga.

## 4. Em que momento ocorreu a condição anormal?

Em 23/09/2026, das 21:03:51 às 21:10:11: 6 minutos, 39 leituras seguidas fora
do normal, 33 delas já na faixa crítica. Foi na M01, puxada pela temperatura,
que chegou aos 83,3 °C. No mesmo intervalo a vibração foi a 5,8 mm/s, a
corrente a 11,7 A, a rotação caiu para 3221 RPM e a vazão despencou de 33 para
11,3 L/min.

Dá para ver dando zoom no painel de temperatura; o painel de eventos de alerta
delimita a janela.

## 5. O evento poderia indicar uma falha?

Sim. Temperatura acima de 80 °C sustentada, com corrente alta e rotação
caindo, é sobrecarga ou atrito: rolamento gasto, selo mecânico apertado. A
vibração crítica ao mesmo tempo aponta para cavitação, que é quando bolhas de
vapor se formam e implodem dentro do rotor; é a falha clássica de bomba
centrífuga. Corrente muito baixa, ao contrário, indicaria bomba trabalhando a
seco, com perda de escorva.

No nosso evento os cinco sinais se moveram juntos na direção certa para
sobrecarga com cavitação: a vazão caiu pela metade, o motor passou a puxar
mais corrente para manter a rotação, e aqueceu. É o tipo de padrão que a
manutenção preditiva tenta pegar antes da quebra.

## 6. Qual informação deveria gerar um alerta?

Essa resposta está implementada, não só descrita: há duas regras ativas no
Grafana, em `infra/grafana/provisioning/alerting/alertas.yaml`, uma para
temperatura acima de 80 °C e outra para vibração acima de 5 mm/s, ambas com
uma instância por máquina. Detalhes em [`04-dashboard.md`](04-dashboard.md).

O critério geral é qualquer sinal entrando na faixa crítica: temperatura acima
de 80 °C, vibração acima de 5 mm/s, corrente acima de 11 A, rotação fora de
3300-3700 RPM, gás acima de 60 %. Além disso, vale alertar quando a máquina
fica em atenção por tempo demais (acima de 2 min), quando some da rede (o LWT
do MQTT marca `offline`) e quando a umidade da casa de bombas sobe muito, que
é indício de vazamento.

Vale separar dois níveis: o alerta de borda, que é o LED vermelho aceso na
hora pela própria placa, e o alerta de plataforma, que é a regra do Grafana,
com histórico e possibilidade de notificar alguém.

## 7. Qual dado deveria ser armazenado por mais tempo?

Os eventos de alerta e os agregados históricos, tipo média por hora. São eles
que sustentam auditoria e manutenção preditiva: para saber se uma bomba está
piorando ao longo de meses, a média horária basta. A telemetria bruta a cada
10 segundos tem valor por pouco tempo e pode expirar; no nosso bucket ela
vive 30 dias (ver [`03-modelo-de-dados.md`](03-modelo-de-dados.md)).

## 8. Qual informação poderia ser processada no edge?

A classificação do status, e o projeto já faz isso: a ESP32 compara cada
leitura com os limiares de `config.h` e aciona o LED RGB da M01 ou o alarme da
M03 na hora, sem depender da rede. Numa demonstração é fácil de mostrar:
segurando o DHT22 entre os dedos o LED muda de cor antes de o dashboard
atualizar.

Outras coisas que caberiam na borda: filtrar ruído e calcular RMS da vibração
em vez de mandar amostra bruta; reduzir a taxa de envio quando o sinal está
estável; e descartar outlier isolado (uma leitura absurda entre duas normais
quase sempre é falha de leitura, não do equipamento).

## 9. Quais dados deveriam ser protegidos?

Credenciais em primeiro lugar: senha do Wi-Fi, usuários do broker, tokens do
InfluxDB e do Grafana. Neste repositório eles ficam fora do Git, em
`secrets.h` e `.env`, e só os arquivos `.example` são versionados.

Depois a telemetria em si. Estação de bombeamento de água é infraestrutura
crítica: a série temporal mostra o padrão operacional do abastecimento, e quem
conseguisse escrever no tópico poderia mascarar uma falha ou provocar uma
parada. Em produção isso pede TLS no MQTT e no HTTP.

E o acesso ao dashboard, com autenticação e papéis separados: operador só
enxerga, administrador edita.

Mais detalhes em [`06-seguranca.md`](06-seguranca.md).

## 10. Qual seria a arquitetura escolhida para uma implantação real?

A mesma topologia, endurecida e com redundância. Como o protótipo já roda todo
em contêiner, levar para servidor é mais trocar endereço e credencial do que
reescrever código. O que mudaria:

- MQTT sobre TLS, com certificado por dispositivo, em vez de acesso anônimo;
- broker em cluster ou gerenciado (EMQX, HiveMQ, AWS IoT Core) no lugar de um
  Mosquitto só;
- ingestão rodando em mais de uma réplica, com uma fila na frente (Kafka, por
  exemplo) para aguentar pico sem perder mensagem;
- banco gerenciado, com backup e retenção em camadas: bruto por dias,
  agregado por anos;
- Grafana com SSO, notificação por e-mail ou Telegram e escala de plantão;
- monitoramento da própria plataforma: se a ingestão morre às 3 da manhã,
  alguém precisa ficar sabendo.

O protótipo mantém os mesmos papéis dessa arquitetura, cada peça com um
equivalente direto do lado de produção.
