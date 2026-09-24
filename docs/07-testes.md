# 07 - Testes

Plano de testes do protótipo. Marcar "ok" ou "falhou" e anotar evidências (prints em
`docs/assets/`); esta tabela vira a seção "testes" do documento técnico.

## T1: Infraestrutura

| ID | Teste | Procedimento | Resultado esperado | Status |
|---|---|---|---|---|
| T1.1 | Subida da stack | `docker compose --project-directory infra up -d` | 4 contêineres saudáveis (`docker compose ps`) | ok (21/09) |
| T1.2 | Broker aceita conexão | `docker exec mosquitto mosquitto_sub -t 'fabrica/#' -C 1 -W 10` com simulador ativo | Mensagem JSON recebida | [ ] |
| T1.3 | Grafana provisionado | Abrir http://localhost:3000 | Dashboard "Mini Central" já existente com datasource ok | ok (21/09: datasource "OK, 3 buckets", dashboard e 2 alertas carregados) |

## T2: Simulador e ingestão

| ID | Teste | Procedimento | Resultado esperado | Status |
|---|---|---|---|---|
| T2.1 | Publicação simulada | `python backend/simulator/simulador.py --duracao 60 --intervalo 5` | Logs de publicação das 3 máquinas | [ ] |
| T2.2 | Gravação no banco | Painéis do Grafana após T2.1 | Séries das 3 máquinas visíveis | ok (21/09: painel 1 com M01/M02/M03, painel 6 = 3) |
| T2.3 | Backfill 500+ | `--backfill 2h --intervalo 10` e depois `python backend/tools/exportar_csv.py` | CSV com ≥ 500 linhas | ok (21/09: 2160 publicados = 2160 gravados = 2160 no CSV) |
| T2.4 | Anomalia | `--anomalia` | Janela crítica visível; painel de alertas > 0 | ok (21/09: M01 08:08 a 08:13, 36 alertas, pico 82,6 °C) |
| T2.5 | Payload inválido | Publicar JSON malformado com `mosquitto_pub` | Ingestão loga descarte, não grava e não cai | [ ] |

## T3: Firmware (por ESP32)

| ID | Teste | Procedimento | Resultado esperado | Status |
|---|---|---|---|---|
| T3.1 | Compilação 3 envs | `pio run -d firmware` | `maquina01/02/03` compilam sem erro | ok (21/09, M01 na DevKit) |
| T3.2 | Conexão Wi-Fi + MQTT | Monitor serial (`pio device monitor`) | Conecta ao hotspot e publica a cada 10 s | parcial (21/09: M01 conectou na UaiFai só depois de mudar o hotspot para 2,4 GHz; MQTT pendente de teste com a stack no ar) |
| T3.3 | Leitura DHT22 (nas 3 máquinas) | Comparar serial com termômetro/ambiente | Temperatura plausível; `fonte=hibrido` | ok na M02 e na M03 (21/09: 30,0 °C / 58,6 % e 29,8 °C / 56,5 %, estáveis). Falhou enquanto a M01 estava no S2 Mini: era contato, não o sensor (ver registro). Refazer com a M01 na DevKit |
| T3.4 | Vazão hall (M01 e M02) | Soprar na turbina do sensor | `flow` sai de 0 no serial e no painel de vazão | ok na M02 (21/09: 286 pulsos em 10 s = 28,6 Hz → 0,39 L/min, com o sensor em 3,3 V e sem divisor). M01 pendente |
| T3.5 | Gás MQ (M03) | Isqueiro sem acender (MQ-2) ou álcool (MQ-135), após ~2 min de aquecimento | Pico de gás; status muda | parcial (21/09: leitura estável em ar limpo, 30,8 % / ADC 1260. Falta provocar com isqueiro) |
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

## Registro de 21/09/2026

Ambiente: notebook Windows 11 com Docker Desktop (WSL 2), PlatformIO 6.2,
rede Wi-Fi "UaiFai" (hotspot do celular, precisou ser colocado em 2,4 GHz:
a ESP32 não enxerga 5 GHz).

- Gargalo encontrado e corrigido no T2.3: o backfill do simulador publica 2160
  mensagens em rajada e a ingestão gravava uma a uma; o Mosquitto descartava o
  que passava da fila padrão (1000) e só 1247 registros chegavam ao banco. Com
  `max_queued_messages 20000` no broker e escrita em lote na ingestão
  (`WriteOptions(batch_size=200, flush_interval=1000)`), 2160/2160.
- M01 na DevKit: gravação por USB funciona, mas o auto-reset falha em parte
  das tentativas ("Wrong boot mode 0x17"); repetir o upload resolve.
- O DHT22 que não lia na M01 não estava queimado. As duas LOLIN S2 Mini vieram
  com a barra de pinos solta, e jumper enfiado no furo sem solda não fecha
  contato: uma varredura dos 12 pinos da fileira externa mostrou tudo em nível
  alto e nenhum pino respondendo como DHT22. O mesmo sensor leu de primeira no
  ESP32-S3, que já vem com header soldado. Lição para a bancada: soldar a barra
  antes de acusar sensor.
- Gravação por placa: a DevKit às vezes precisa do BOTÃO BOOT segurado durante
  o "Connecting..."; o S3 grava sozinho pela porta USB-C marcada `UART`.

## Registro de 23/09/2026

Revisão do código com a stack ainda fora do ar (o notebook usado nesta sessão
não tem Docker instalado). O que foi verificado sem broker:

- `pio run` compila os três environments (`maquina01`, `maquina02`,
  `maquina03`) sem aviso;
- a lógica de status da ingestão foi exercitada com os payloads reais
  capturados no serial em 21/09 e com casos de borda (JSON quebrado, máquina
  fora da lista, campo como texto): todos os inválidos são descartados com log
  e nenhum derruba o serviço;
- a janela de anomalia do simulador foi conferida nos dois modos: no backfill
  continua a 80 % do percurso e, em tempo real sem `--duracao`, agora se repete
  a cada 15 min por 60 s, como o firmware.

Três defeitos corrigidos nesta revisão (detalhe em cada arquivo):

1. toda a telemetria estava sendo publicada com a flag **retida** do MQTT, por
   causa de uma sobrecarga do PubSubClient que casava `publish(topico, payload,
   tamanho)` com a versão `(topico, string, retained)`. O broker guardava a
   última leitura de cada máquina e reentregava para qualquer assinante novo:
   ao reiniciar a ingestão, leitura velha voltava para o banco;
2. o `--anomalia` do simulador não fazia nada no modo contínuo, que é
   justamente o plano B da apresentação;
3. os limiares de gás (20 %) e o offset de temperatura (+45) deixavam M03 e M02
   permanentemente em "atenção" com tudo funcionando normalmente.

Ainda pendente: T1.2, T2.1, T2.5, T3.2 (com a stack no ar), T3.6, T3.7, T3.8 e
todo o T4.
