# 05 - Análise (Desafio 4)

As respostas 1 a 4 dependem dos dados coletados: preencher os campos marcados após a
coleta, consultando o dashboard (instruções em cada item). As demais já estão
respondidas com base na arquitetura.

## 1. Qual máquina apresentou maior temperatura?

**Resposta (preencher com os dados coletados):** `M__`, com pico de `__,_ °C` em `__/__ às __:__`.

> Como obter: painel "Temperatura ao longo do tempo" → inspecionar máximo por
> série; ou Flux `max()` agrupado por `machine`.

## 2. Qual máquina apresentou maior vibração?

**Resposta (preencher com os dados coletados):** `M__`, com pico de `_,_ mm/s`.

> Painel "Vibração ao longo do tempo", mesmo procedimento.

## 3. Existe relação entre temperatura e corrente?

**Resposta (preencher com os dados coletados):** `Sim/Não, descrever`.

**Fundamentação esperada:** sim: em motores, aumento de carga eleva a corrente
(efeito Joule, I²R) e consequentemente a temperatura. O simulador reproduz essa
correlação (anomalia térmica acompanha elevação de corrente). Comparar as duas
séries da mesma máquina no mesmo intervalo do dashboard.

## 4. Em que momento ocorreu a condição anormal?

**Resposta (preencher com os dados coletados):** `__/__/____ às __:__`, máquina `M__`, sinal `temperatura`,
atingindo `__ °C` (faixa crítica: > 80 °C).

> Zoom no painel de temperatura; o painel "Eventos de alerta" delimita a janela.

## 5. O evento poderia indicar uma falha?

Sim. No contexto da motobomba, temperatura acima de 80 °C sustentada com
corrente elevada e queda de RPM indica sobrecarga/atrito (desgaste de
rolamento ou selo mecânico); vibração crítica simultânea apontaria para
**cavitação** (bolhas de vapor implodindo no rotor, falha clássica de bombas
centrífugas); corrente muito baixa indicaria **operação a seco** (perda de
escorva). No nosso evento, o padrão observado sugere
`completar com o observado`. É exatamente o tipo de evento que a manutenção
preditiva busca antecipar.

## 6. Qual informação deveria gerar um alerta?

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
depender da rede. Também são candidatos: filtragem/RMS da vibração (já feito no
M02), downsampling e detecção de outliers antes do envio.

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
