# 04 - Dashboard (Desafio 3)

Dashboard **"Mini Central de Monitoramento IoT"** provisionado como código em
`infra/grafana/provisioning/dashboards/mini-central.json`, carregado
automaticamente ao subir o Grafana (`docker compose up`).

## Painéis (mínimos exigidos + extras)

| # | Painel | Tipo | Exigido? |
|---|---|---|---|
| 1 | Temperatura atual (por máquina) | Stat com thresholds | Sim |
| 2 | Temperatura ao longo do tempo | Time series | Sim |
| 3 | Vibração ao longo do tempo | Time series | Sim |
| 4 | Corrente elétrica | Time series | Sim |
| 5 | Rotação da máquina (RPM) | Time series | Sim |
| 6 | Máquinas em condição normal | Stat | Sim |
| 7 | Eventos de alerta (janela atual) | Stat | Sim |
| 8 | Gás na casa de bombas (M03) | Gauge | extra |
| 9 | Umidade da casa de bombas (por máquina) | Stat | extra |
| 10 | Últimas leituras | Table | extra |
| 11 | Vazão da motobomba (L/min) | Time series | extra |

Thresholds coloridos seguem as faixas de
[`03-modelo-de-dados.md`](03-modelo-de-dados.md): verde (normal), amarelo
(atenção), vermelho (crítico).

## Consultas Flux de referência

Temperatura ao longo do tempo (padrão para os painéis 2 a 5, trocando `_field`):

```flux
from(bucket: "iot")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "telemetria" and r._field == "temperature")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> keep(columns: ["_time", "_value", "machine"])
```

Máquinas em condição normal (painel 6, último status de cada máquina). Os
painéis de valor atual usam a janela do próprio dashboard, e não uma janela
fixa, para nunca ficarem em branco durante a apresentação:

```flux
from(bucket: "iot")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "telemetria" and r._field == "status_code")
  |> group(columns: ["machine"])
  |> last()
  |> filter(fn: (r) => r._value == 0)
  |> group()
  |> count()
```

Eventos de alerta na janela selecionada (painel 7):

```flux
from(bucket: "iot")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "telemetria" and r._field == "status_code")
  |> filter(fn: (r) => r._value >= 1)
  |> group()
  |> count()
```

## Convenções visuais

- Refresh automático: 5 s (apresentação ao vivo);
- Janela padrão: última 1 hora;
- Uma cor fixa por máquina em todos os painéis (M01 azul, M02 laranja,
  M03 roxo) para leitura imediata;
- Linha de threshold desenhada nos gráficos de temperatura (75 °C e 80 °C).

## Evidências para a entrega

Capturar e salvar em `docs/assets/`:

1. Dashboard completo com as 3 máquinas em condição normal;
2. Dashboard durante a anomalia (painéis 6/7 reagindo, série vermelha);
3. Zoom no momento exato da anomalia (usado no Desafio 4, pergunta 4).
