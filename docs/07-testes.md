# 07 - Testes

Plano de testes do protótipo. Marcar "ok" ou "falhou" e anotar evidências (prints em
`docs/assets/`); esta tabela vira a seção "testes" do documento técnico.

## T1: Infraestrutura

| ID | Teste | Procedimento | Resultado esperado | Status |
|---|---|---|---|---|
| T1.1 | Subida da stack | `docker compose --project-directory infra up -d` | 4 contêineres saudáveis (`docker compose ps`) | [ ] |
| T1.2 | Broker aceita conexão | `docker exec mosquitto mosquitto_sub -t 'fabrica/#' -C 1 -W 10` com simulador ativo | Mensagem JSON recebida | [ ] |
| T1.3 | Grafana provisionado | Abrir http://localhost:3000 | Dashboard "Mini Central" já existente com datasource ok | [ ] |

## T2: Simulador e ingestão

| ID | Teste | Procedimento | Resultado esperado | Status |
|---|---|---|---|---|
| T2.1 | Publicação simulada | `python backend/simulator/simulador.py --duracao 60 --intervalo 5` | Logs de publicação das 3 máquinas | [ ] |
| T2.2 | Gravação no banco | Painéis do Grafana após T2.1 | Séries das 3 máquinas visíveis | [ ] |
| T2.3 | Backfill 500+ | `--backfill 2h --intervalo 10` e depois `python backend/tools/exportar_csv.py` | CSV com ≥ 500 linhas | [ ] |
| T2.4 | Anomalia | `--anomalia` | Janela crítica visível; painel de alertas > 0 | [ ] |
| T2.5 | Payload inválido | Publicar JSON malformado com `mosquitto_pub` | Ingestão loga descarte, não grava e não cai | [ ] |

## T3: Firmware (por ESP32)

| ID | Teste | Procedimento | Resultado esperado | Status |
|---|---|---|---|---|
| T3.1 | Compilação 3 envs | `pio run -d firmware` | `maquina01/02/03` compilam sem erro | [ ] |
| T3.2 | Conexão Wi-Fi + MQTT | Monitor serial (`pio device monitor`) | Conecta ao hotspot e publica a cada 10 s | [ ] |
| T3.3 | Leitura DHT22 (nas 3 máquinas) | Comparar serial com termômetro/ambiente | Temperatura plausível; `fonte=hibrido` | [ ] |
| T3.4 | Vazão hall (M01 e M02) | Soprar na turbina do sensor | `flow` sai de 0 no serial e no painel de vazão | [ ] |
| T3.5 | Gás MQ (M03) | Isqueiro sem acender (MQ-2) ou álcool (MQ-135), após ~2 min de aquecimento | Pico de gás; status muda | [ ] |
| T3.6 | Edge/LEDs | Forçar leitura crítica (aquecer DHT22 com o dedo com `TEMP_OFFSET` de demo) | LED RGB muda verde→amarelo→vermelho; HW-481 pisca no crítico | [ ] |
| T3.7 | Queda de rede | Desligar hotspot por 30 s | ESP32 reconecta sozinho e volta a publicar | [ ] |
| T3.8 | LWT | Desligar uma ESP32 | Tópico `status` marca `offline` | [ ] |

## T4: Ensaio da apresentação

| ID | Teste | Procedimento | Resultado esperado | Status |
|---|---|---|---|---|
| T4.1 | Demo fim-a-fim no hotspot | 3 ESP32 + stack no notebook, sem internet externa | Dashboard atualizando ao vivo | [ ] |
| T4.2 | Anomalia ao vivo | Aquecer DHT22 / aproximar gás do MQ durante a demo | Alerta aparece em < 15 s no dashboard | [ ] |
| T4.3 | Plano B | Desligar as ESP32 e ligar o simulador | Demo continua sem hardware | [ ] |
| T4.4 | Tempo | Ensaiar apresentação completa | ≤ 10 min | [ ] |
