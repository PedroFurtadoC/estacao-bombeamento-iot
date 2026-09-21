# 01 - Requisitos da Atividade

Fonte: enunciado "Projeto IoT / Prova Parcial" (UNAERP, Prof. Carlos Formigoni).
Este documento consolida **todos** os requisitos e aponta onde cada um é atendido
neste repositório (rastreabilidade).

## Objetivo

Construir, em grupo (4 a 5 alunos), uma pequena arquitetura IoT que reproduza,
em escala didática, uma arquitetura profissional de monitoramento de fábrica,
com sistema de visualização.

## Cenário

A equipe foi contratada para monitorar uma pequena fábrica com **3 máquinas**
(M01, M02, M03). Cada máquina possui **4 sinais**:

> **Contextualização adotada pelo grupo:** a "fábrica" é uma **estação de
> bombeamento de água** e as 3 máquinas são **conjuntos motobomba** (motor
> elétrico de 2 polos + bomba centrífuga, nominal ~3500 RPM). Essa escolha dá
> significado físico a todos os sinais: temperatura = carcaça do motor;
> vibração = cavitação/desgaste de rolamento (fenômeno acústico); corrente =
> carga do motor (detecta operação a seco); rotação = RPM do conjunto;
> umidade da casa de bombas = indício de vazamento; gás = segurança de espaço
> confinado. Estação de água é infraestrutura crítica, o que enriquece as
> respostas de segurança e de arquitetura real do Desafio 4.

| Sinal | Unidade adotada | Observação |
|---|---|---|
| Temperatura | °C | |
| Vibração | mm/s | O slide de cenário cita "voltagem", porém o JSON de exemplo e os painéis obrigatórios do dashboard usam **vibração**; adotamos o contrato JSON |
| Corrente | A | |
| Rotação | RPM | |

### Contrato JSON de referência (do enunciado)

```json
{
  "machine": "M01",
  "temperature": 72.4,
  "vibration": 2.8,
  "current": 8.2,
  "rotation": 9400,
  "timestamp": "2026-08-20T17:00:00"
}
```

## Pontuação

| Opção | Configuração | Pontos |
|---|---|---|
| A | 1 ESP32 + dados simulados (3 máquinas × 4 sinais) | até 2 |
| B | 1 ESP32 + dados de sensores (9 sinais; sugestão BME280/BMP280) | até 3 |
| **C (escolhida)** | **3 ESP32 + dados de sensores** (1 sensor real por ESP32, ou o sinal que o grupo escolher monitorar) | **até 4** |

**Estratégia adotada (opção C):** cada ESP32 representa uma máquina e possui ao
menos um sensor físico; os sinais sem sensor físico são simulados por software
no próprio dispositivo (prática explicitamente permitida pelo enunciado).

| ESP32 | Sensores físicos | Sinais reais | Sinais simulados no firmware |
|---|---|---|---|
| M01 | DHT22 + sensor de vazão hall | Temperatura (+ umidade), vazão | Vibração, corrente, rotação |
| M02 | DHT22 + sensor de vazão hall | Temperatura (+ umidade), vazão | Vibração, corrente, rotação |
| M03 | DHT22 + MQ (gás) | Temperatura (+ umidade), gás | Vibração, corrente, rotação, vazão |

Todas as máquinas têm **pelo menos 2 sensores reais**: temperatura em todas
(1 DHT22 por ESP32, espelhando a sugestão do professor de "um BME280 para cada
ESP32"), vazão nas M01/M02 e gás na M03, superando com folga o mínimo da opção C. A vazão
é o principal indicador de processo de uma bomba: queda indica
obstrução/cavitação e zero indica operação a seco. Atuadores de borda: LED RGB
HW-479 (semáforo de status) e LED flash HW-481 (alarme em condição crítica),
que demonstram processamento no edge.

## Desafio 1: Arquitetura

Desenhar a arquitetura contendo, com justificativa de **cada** componente:
sensores, dispositivo IoT, comunicação, back-end, banco, Grafana (ou outro
dashboard) e usuário.

Atendido em [`02-arquitetura.md`](02-arquitetura.md).

## Desafio 2: Dados

- Gerar **pelo menos 500 registros**;
- com **variação normal**, **pequenas oscilações** e **pelo menos uma situação
  anormal**.

Faixas de referência do enunciado (temperatura):

| Faixa | Valor |
|---|---|
| Normal | 65-75 °C |
| Atenção | 75-80 °C |
| Crítica | > 80 °C |

Atendido pelo firmware (simulação híbrida) e pelo simulador
`backend/simulator/simulador.py` (backfill + injeção de anomalia). Faixas de
todos os sinais em [`03-modelo-de-dados.md`](03-modelo-de-dados.md).

## Desafio 3: Dashboard

Dashboard com **no mínimo** os painéis (pode incluir mais):

1. Temperatura atual
2. Temperatura ao longo do tempo
3. Vibração ao longo do tempo
4. Corrente elétrica
5. Rotação da máquina (RPM)
6. Quantidade de máquinas em condição normal
7. Quantidade de eventos de alerta

Atendido pelo dashboard provisionado em
`infra/grafana/provisioning/dashboards/mini-central.json` e documentado em
[`04-dashboard.md`](04-dashboard.md).

## Desafio 4: Análise

Responder às 10 perguntas (máquina mais quente, maior vibração, relação
temperatura × corrente, momento da anomalia, falha, alertas, retenção, edge,
proteção de dados e arquitetura real).

Atendido em [`05-analise.md`](05-analise.md).

## Entrega final (DOC/PDF via Classroom, 1 integrante envia)

1. Diagrama da arquitetura;
2. Base de dados;
3. Dashboard;
4. **Documento técnico** com: introdução; problema; arquitetura; tecnologias;
   modelo de dados; segurança; dashboard; testes; resultados; conclusão.

Apresentação: seminário em PowerPoint, **10 min + 5 min de perguntas**, com o
**protótipo funcionando** e o dashboard ao vivo.

> Observação do professor: **não depender do Wi-Fi da universidade**: usar
> o hotspot de um notebook para as ESP32.

Mapa de montagem do documento final em
[`08-documento-tecnico.md`](08-documento-tecnico.md).

## Checklist de rastreabilidade

- [ ] 3 ESP32 publicando telemetria (firmware gravado e testado)
- [ ] 1 sensor físico por ESP32 conectado e lido
- [ ] ≥ 500 registros no InfluxDB (verificar com `backend/tools/exportar_csv.py`)
- [ ] Pelo menos 1 anomalia registrada e visível no dashboard
- [ ] 7 painéis mínimos funcionando no Grafana
- [ ] Regras de alerta ativas (temperatura e vibração críticas)
- [ ] 10 perguntas do Desafio 4 respondidas com base nos dados reais coletados
- [ ] Documento técnico montado (10 seções) em DOC/PDF
- [ ] Slides prontos (10 min) + ensaio com protótipo e hotspot
