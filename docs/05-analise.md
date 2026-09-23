# 05 - Análise (Desafio 4)

As respostas 1 a 5 usam a base de dados coletada em **21/09/2026** (2160
registros, 720 por máquina, exportados em `dados/export/telemetria.csv` pelo
`backend/tools/exportar_csv.py`): backfill de 2 h do simulador com a janela de
anomalia na M01. Se a base for gerada de novo (por exemplo com as ESP32 no dia
da apresentação), atualize os números pelos mesmos painéis/consultas indicados.
As demais respostas se baseiam na arquitetura.

## 1. Qual máquina apresentou maior temperatura?

**Resposta:** **M01**, com pico de **82,6 °C** em **21/09/2026 às 08:13:51**.
M02 e M03 não passaram de 74,1 °C e 75,0 °C, dentro da faixa normal.

> Como obter: painel "Temperatura ao longo do tempo" → inspecionar máximo por
> série; ou Flux `max()` agrupado por `machine`.

## 2. Qual máquina apresentou maior vibração?

**Resposta:** **M01**, com pico de **5,8 mm/s** às 08:11:21 (faixa crítica:
acima de 5 mm/s). M02 e M03 ficaram em no máximo 3,4 e 3,2 mm/s.

> Painel "Vibração ao longo do tempo", mesmo procedimento.

## 3. Existe relação entre temperatura e corrente?

**Resposta:** **Sim, correlação positiva.** Na M01 o coeficiente de Pearson
entre temperatura e corrente nas 720 leituras foi **r = 0,63**. Comparando as
fases da mesma máquina:

| Fase (M01) | Leituras | Temperatura | Corrente | Rotação | Vibração | Vazão |
|---|---|---|---|---|---|---|
| Normal | 684 | 68,1 °C | 8,1 A | 3494 RPM | 1,65 mm/s | 30,0 L/min |
| Anomalia | 36 | 81,6 °C | 11,4 A | 3258 RPM | 5,40 mm/s | 12,5 L/min |

**Fundamentação:** em motores, aumento de carga eleva a corrente e, por efeito
Joule (I²R), a temperatura. As duas séries sobem juntas na anomalia enquanto a
rotação cai, assinatura de sobrecarga/atrito.

## 4. Em que momento ocorreu a condição anormal?

**Resposta:** em **21/09/2026, das 08:08:01 às 08:13:51** (6 minutos, 36
leituras consecutivas em alerta, 34 delas críticas), máquina **M01**, sinal
**temperatura**, atingindo **82,6 °C** (faixa crítica: > 80 °C), acompanhada de
vibração até 5,8 mm/s, corrente até 11,4 A e rotação caindo a 3234 RPM.

> Zoom no painel de temperatura; o painel "Eventos de alerta" delimita a janela.

## 5. O evento poderia indicar uma falha?

Sim. No contexto da motobomba, temperatura acima de 80 °C sustentada com
corrente elevada e queda de RPM indica sobrecarga/atrito (desgaste de
rolamento ou selo mecânico); vibração crítica simultânea apontaria para
**cavitação** (bolhas de vapor implodindo no rotor, falha clássica de bombas
centrífugas); corrente muito baixa indicaria **operação a seco** (perda de
escorva). No nosso evento, o padrão observado (temperatura e corrente altas,
rotação em queda, vibração crítica e vazão caindo de 30 para 12,5 L/min) sugere
**sobrecarga com cavitação**: o rotor perde vazão, o motor trabalha mais e
aquece. É exatamente o tipo de evento que a manutenção preditiva busca
antecipar.

## 6. Qual informação deveria gerar um alerta?

> Esta resposta está **implementada**, não só descrita: há duas regras de
> alerta ativas no Grafana, provisionadas em
> `infra/grafana/provisioning/alerting/alertas.yaml`, uma para temperatura
> acima de 80 °C e outra para vibração acima de 5 mm/s, ambas com uma instância
> por máquina. Detalhes em [`04-dashboard.md`](04-dashboard.md).

Qualquer sinal na faixa **crítica** (temperatura > 80 °C, vibração > 5 mm/s,
corrente > 11 A, rotação fora de 3300-3700 RPM, gás > 40 %) e também a
**transição** para atenção quando persistente (> 2 min), além de máquina
**offline** (LWT do MQTT) e umidade anormalmente alta na casa de bombas
(possível vazamento). Alertas de borda: LED vermelho/alarme local; alertas de
plataforma: regra no Grafana.

## 7. Qual dado deveria ser armazenado por mais tempo?

Os **eventos de alerta/anomalias** e os **agregados históricos** (médias
horárias): são a base de auditoria e manutenção preditiva. A telemetria bruta
de alta frequência pode expirar em ~30 dias (política de retenção do bucket);
detalhes em [`03-modelo-de-dados.md`](03-modelo-de-dados.md).

## 8. Qual informação poderia ser processada no edge?

A **classificação do status** (normal/atenção/crítico), e este projeto **já
faz isso**: o ESP32 compara cada leitura com os limiares e aciona o LED RGB
(HW-479) / alarme (HW-481) localmente, com latência de milissegundos e sem
depender da rede. Também são candidatos: filtragem/RMS da vibração,
downsampling e detecção de outliers antes do envio.

## 9. Quais dados deveriam ser protegidos?

- **Credenciais**: Wi-Fi, usuários do broker, tokens do InfluxDB e do Grafana
  (neste repo ficam fora do Git: `secrets.h`, `.env`);
- **Telemetria em trânsito e em repouso**: uma estação de bombeamento de água
  é **infraestrutura crítica**: telemetria revela padrão operacional do
  abastecimento e um ataque poderia mascarar falhas ou parar bombas. Em
  produção, TLS no MQTT e no HTTP;
- **Acesso ao dashboard**: autenticação e papéis (operador × administrador).

Detalhes em [`06-seguranca.md`](06-seguranca.md).

## 10. Qual seria a arquitetura escolhida para uma implantação real?

A mesma topologia, endurecida e escalada. Como o protótipo roda inteiro em
Docker, levá-lo para servidores é trocar o endereço do broker, sem alterar
código. Em produção:

- ESP32 industriais (ou gateways) com **MQTT sobre TLS** e autenticação por
  certificado por dispositivo;
- Broker gerenciado/cluster (EMQX, HiveMQ ou AWS IoT Core);
- Ingestão como serviço redundante (containers orquestrados, Kubernetes) com
  fila de amortecimento (por ex. Kafka) para picos;
- InfluxDB/TimescaleDB gerenciado com backup e retenção por camadas
  (bruto → agregado);
- Grafana corporativo com SSO, alertas para e-mail/Telegram e on-call;
- Observabilidade da própria plataforma (logs e métricas da ingestão).

O protótipo didático preserva 1:1 os papéis dessa arquitetura: cada peça tem
um equivalente direto de produção.
