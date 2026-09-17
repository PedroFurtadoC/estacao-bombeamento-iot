# 11 - Começando do Zero

Guia para quem vai rodar o projeto pela primeira vez, numa máquina onde nada
está instalado. Do clone até o dashboard funcionando leva cerca de 15 minutos,
sendo quase tudo download.

**Você não precisa de ESP32 nem de sensor nenhum para rodar isto.** O simulador
gera os dados das três motobombas e o sistema funciona inteiro. O hardware entra
depois, e o passo a passo dele está em
[`09-guia-de-bancada.md`](09-guia-de-bancada.md).

## 1. Instalar o que é necessário

| Programa | Para quê | Onde baixar |
|---|---|---|
| Docker Desktop | sobe o broker, o banco, o back-end e o dashboard | <https://www.docker.com/products/docker-desktop/> |
| Python 3.11 ou mais novo | roda o simulador e a exportação | <https://www.python.org/downloads/> |
| Git | clona o repositório | <https://git-scm.com/downloads> |

Na instalação do Python, marque a opção **Add Python to PATH**. Depois de
instalar o Docker Desktop, abra o programa uma vez e espere o ícone da baleia
ficar estável, senão os comandos falham dizendo que não encontram o Docker.

Confira se deu certo:

```powershell
docker --version
python --version
git --version
```

## 2. Clonar o projeto

```powershell
git clone https://github.com/PedroFurtadoC/estacao-bombeamento-iot.git
cd estacao-bombeamento-iot
```

## 3. Criar o arquivo de senhas

O repositório não traz senhas, só um modelo. Crie o seu:

```powershell
Copy-Item infra/.env.example infra/.env
```

Abra `infra/.env` e troque os valores que começam com `troque`. Qualquer texto
serve, desde que você não deixe o padrão. Esse arquivo nunca vai para o Git.

## 4. Subir a stack

```powershell
docker compose --project-directory infra up -d --build
```

Na primeira vez o Docker baixa as imagens, o que pode levar alguns minutos.
Quando terminar, confira:

```powershell
docker compose --project-directory infra ps
```

Os quatro contêineres precisam estar de pé: `mosquitto`, `influxdb`,
`ingestao` e `grafana`.

Isso sobe quatro serviços de uma vez:

| Serviço | Porta | Papel |
|---|---|---|
| Mosquitto | 1883 | broker MQTT, recebe as mensagens |
| InfluxDB | 8086 | banco de séries temporais |
| Ingestão | interno | valida cada leitura e grava no banco |
| Grafana | 3000 | dashboard |

## 5. Gerar dados

```powershell
python -m pip install -r backend/simulator/requirements.txt
python backend/simulator/simulador.py --backfill 50m --intervalo 10 --anomalia
```

Isso publica 900 leituras das três motobombas, cobrindo os últimos 50 minutos,
já com uma anomalia na M01. Atende o requisito de 500 registros do Desafio 2.

Para deixar publicando ao vivo, o que é útil na apresentação:

```powershell
python backend/simulator/simulador.py --intervalo 10
```

## 6. Abrir o dashboard

Vá em <http://localhost:3000>. O usuário e a senha são os que você colocou no
`infra/.env`.

Você não precisa importar nada nem criar painel: o dashboard, a conexão com o
banco e as regras de alerta já sobem configurados junto com a stack. Isso é
proposital, para que todo mundo do grupo veja exatamente a mesma tela.

O que você deve ver:

- **Visão geral** com a temperatura atual das três máquinas, quantas estão em
  condição normal e quantos eventos de alerta houve na janela;
- **Sinais ao longo do tempo** com temperatura, vibração, corrente, rotação e
  vazão. Por volta do fim da janela dá para ver a anomalia da M01: temperatura,
  corrente e vibração sobem enquanto rotação e vazão caem;
- **Detalhe das leituras** com a tabela das últimas medições.

No topo tem o seletor **Motobomba**, que filtra tudo por máquina.

Os painéis de umidade e gás aparecem como "aguardando sensor", e isso está
certo: o simulador não gera esses dois campos, que vêm dos sensores físicos.

## 7. Conferir os alertas

Menu lateral, **Alerting**, **Alert rules**. Existem duas regras ativas:

| Regra | Dispara quando |
|---|---|
| Temperatura critica na motobomba | temperatura passa de 80 °C por 1 minuto |
| Vibracao critica (possivel cavitacao) | vibração passa de 5 mm/s por 1 minuto |

Cada uma cria uma instância por máquina, então o alerta diz qual motobomba
está em falha. Se nenhuma ESP32 estiver publicando, a regra fica em `NoData`,
o que também serve para indicar dispositivo offline.

## 8. Exportar a base de dados

Serve para anexar na entrega:

```powershell
python -m pip install -r backend/tools/requirements.txt
python backend/tools/exportar_csv.py --inicio -7d
```

O arquivo sai em `dados/export/telemetria.csv` e o script avisa se o total
ficou abaixo dos 500 registros exigidos.

## 9. Parar e voltar

```powershell
docker compose --project-directory infra stop     # pausa, mantém os dados
docker compose --project-directory infra start    # volta de onde parou
docker compose --project-directory infra down     # remove os contêineres, mantém os dados
docker compose --project-directory infra down -v  # remove tudo, inclusive os dados
```

Use o `-v` só quando quiser mesmo começar do zero.

## Se der problema

| Sintoma | Causa | O que fazer |
|---|---|---|
| `failed to connect to the docker API` | Docker Desktop fechado | Abrir o Docker Desktop e esperar o ícone estabilizar |
| `port is already allocated` | outra coisa usando 3000, 8086 ou 1883 | Fechar o outro programa ou trocar a porta no `docker-compose.yml` |
| Grafana pede login e a senha não entra | `infra/.env` criado depois da stack subir | `docker compose --project-directory infra down -v` e subir de novo |
| Dashboard aberto mas sem dado | janela de tempo sem leitura | Rodar o simulador de novo, ou aumentar a janela no canto superior direito |
| Painéis de baixo em branco | o Grafana só carrega painel que entra na tela | Rolar a página até eles |
| `ModuleNotFoundError: paho` | dependência do simulador faltando | `python -m pip install -r backend/simulator/requirements.txt` |

## Onde mexer em cada coisa

O projeto inteiro é configurado por arquivo, e não pela interface. Mudança
feita direto no Grafana funciona na hora, mas some quando a stack é recriada e
não chega para o resto do grupo. O certo é editar o arquivo e recarregar.

| O que mudar | Arquivo |
|---|---|
| Painéis do dashboard | `infra/grafana/provisioning/dashboards/mini-central.json` |
| Regras de alerta | `infra/grafana/provisioning/alerting/alertas.yaml` |
| Conexão com o banco | `infra/grafana/provisioning/datasources/influxdb.yml` |
| Limiares usados pelo back-end | `backend/ingest/main.py` |
| Comportamento do simulador | `backend/simulator/simulador.py` |
| Firmware das ESP32 | `firmware/src/` e `firmware/platformio.ini` |

O Grafana recarrega dashboards a cada 30 segundos. Para alertas e datasource,
reinicie o serviço:

```powershell
docker compose --project-directory infra restart grafana
```

## Próximos passos

Com a stack rodando, o que falta é o hardware. O caminho é
[`09-guia-de-bancada.md`](09-guia-de-bancada.md) para o roteiro de montagem e
[`10-esquema-eletrico.md`](10-esquema-eletrico.md) para a ligação pino a pino
das três placas.
