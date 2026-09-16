# 08 - Documento Técnico (montagem da entrega)

O documento final (DOC/PDF enviado ao Classroom por 1 integrante) deve conter
as 10 seções abaixo. Este arquivo mapeia cada seção à sua fonte no repositório;
montar é consolidar, não reescrever.

| # | Seção exigida | Fonte no repositório | Observações |
|---|---|---|---|
| 1 | Introdução | Escrever (2 a 3 parágrafos) | Contexto IoT industrial + objetivo do trabalho; basear-se no README |
| 2 | Problema | `01-requisitos.md` (cenário) | Fábrica com 3 máquinas, 4 sinais, necessidade de monitoramento e alerta |
| 3 | Arquitetura | `02-arquitetura.md` | Incluir o diagrama (exportar o mermaid como imagem) e a justificativa de cada componente |
| 4 | Tecnologias | `02-arquitetura.md` (tabela de decisões) + README | ESP32, PlatformIO, MQTT/Mosquitto, Python, InfluxDB, Grafana, Docker |
| 5 | Modelo de dados | `03-modelo-de-dados.md` | Contrato JSON, tópicos, schema, faixas e retenção |
| 6 | Segurança | `06-seguranca.md` | Implementado × produção |
| 7 | Dashboard | `04-dashboard.md` + prints de `docs/assets/` | Os 7 painéis + extras |
| 8 | Testes | `07-testes.md` preenchido | Tabelas com resultados reais |
| 9 | Resultados | `05-analise.md` preenchido + export CSV | Nº de registros, anomalia registrada, respostas 1 a 4 com dados reais |
| 10 | Conclusão | Escrever (1 a 2 parágrafos) | O que funcionou, dificuldades, aprendizados, evolução futura |

## Demais itens da entrega

| Item | Como gerar |
|---|---|
| Diagrama da arquitetura | Renderizar o mermaid de `02-arquitetura.md` (GitHub renderiza; exportar print de alta resolução) |
| Base de dados | `python backend/tools/exportar_csv.py` → CSV com ≥ 500 registros |
| Dashboard | Prints + JSON exportado (`infra/grafana/provisioning/dashboards/mini-central.json`) |
| Apresentação (PowerPoint, 10 min) | Roteiro sugerido abaixo |

## Roteiro sugerido dos slides (10 min)

1. Capa + equipe (30 s)
2. Problema e cenário (1 min)
3. Arquitetura: diagrama e justificativas (2 min)
4. Hardware: 3 ESP32 + sensores reais e simulação híbrida (1,5 min)
5. Pipeline de dados: MQTT → ingestão → InfluxDB (1 min)
6. **Demo ao vivo**: dashboard + anomalia provocada no sensor (2,5 min)
7. Análise: principais respostas do Desafio 4 (1 min)
8. Dificuldades e conclusão (30 s)

> Ensaiar com o hotspot do notebook e deixar o simulador pronto como plano B
> (teste T4.3).
