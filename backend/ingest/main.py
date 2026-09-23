"""Serviço de ingestão: assina a telemetria MQTT, valida e grava no InfluxDB.

Camada de back-end da arquitetura (docs/02-arquitetura.md):
MQTT (Mosquitto) -> validação/enriquecimento -> InfluxDB (bucket iot).
"""

from __future__ import annotations

import json
import logging
import os
from datetime import datetime, timedelta, timezone

import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import WriteOptions

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
log = logging.getLogger("ingestao")

MQTT_HOST = os.getenv("MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
TOPICO = os.getenv("MQTT_TOPICO", "fabrica/maquinas/+/telemetria")

INFLUX_URL = os.getenv("INFLUX_URL", "http://localhost:8086")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN", "")
INFLUX_ORG = os.getenv("INFLUX_ORG", "unaerp")
INFLUX_BUCKET = os.getenv("INFLUX_BUCKET", "iot")

FUSO_LOCAL = timezone(timedelta(hours=-3))  # America/Sao_Paulo (sem DST)

MAQUINAS_VALIDAS = {"M01", "M02", "M03"}
CAMPOS_OBRIGATORIOS = ("temperature", "vibration", "current", "rotation")
CAMPOS_OPCIONAIS = ("humidity", "gas", "flow")


def calcular_status(dados: dict) -> int:
    """Espelha a classificação do firmware: 0 normal, 1 atenção, 2 crítico."""
    t, v = dados["temperature"], dados["vibration"]
    c, r = dados["current"], dados["rotation"]
    q = dados.get("flow", 30)
    status = 0
    if t > 80 or v > 5 or c > 11 or r < 3300 or r > 3700 or q < 10 or q > 45 or dados.get("gas", 0) > 40:
        status = 2
    elif (
        t > 75
        or v > 3.5
        or c > 9
        or r < 3400
        or r > 3600
        or q < 20
        or q > 40
        or dados.get("gas", 0) > 20
    ):
        status = 1
    return status


def interpretar_timestamp(bruto: str | None) -> datetime:
    """Timestamp do payload (hora local UTC-3, via NTP) ou hora de chegada."""
    if bruto:
        try:
            ts = datetime.fromisoformat(bruto)
            if ts.tzinfo is None:
                ts = ts.replace(tzinfo=FUSO_LOCAL)
            return ts.astimezone(timezone.utc)
        except ValueError:
            log.warning("timestamp inválido %r, usando hora de chegada", bruto)
    return datetime.now(timezone.utc)


def validar(payload: bytes) -> dict | None:
    """Retorna o dicionário validado ou None (payload descartado)."""
    try:
        dados = json.loads(payload)
    except (json.JSONDecodeError, UnicodeDecodeError):
        log.warning("payload não é JSON válido: %r", payload[:120])
        return None
    if not isinstance(dados, dict):
        log.warning("payload não é um objeto JSON: %r", payload[:120])
        return None
    if dados.get("machine") not in MAQUINAS_VALIDAS:
        log.warning("machine inválida: %r", dados.get("machine"))
        return None
    for campo in CAMPOS_OBRIGATORIOS:
        if not isinstance(dados.get(campo), (int, float)) or isinstance(dados.get(campo), bool):
            log.warning("campo %s ausente/não numérico em %s", campo, dados.get("machine"))
            return None
    return dados


def ao_conectar(cliente: mqtt.Client, _userdata, _flags, codigo, _props=None) -> None:
    if codigo == 0:
        cliente.subscribe(TOPICO, qos=1)
        log.info("conectado ao broker %s:%s, assinando %s", MQTT_HOST, MQTT_PORT, TOPICO)
    else:
        log.error("falha na conexão MQTT: %s", codigo)


def criar_callback_mensagem(write_api):
    def ao_receber(_cliente, _userdata, msg: mqtt.MQTTMessage) -> None:
        dados = validar(msg.payload)
        if dados is None:
            return
        status = dados.get("status")
        if not isinstance(status, int) or status not in (0, 1, 2):
            status = calcular_status(dados)

        ponto = (
            Point("telemetria")
            .tag("machine", dados["machine"])
            .tag("fonte", dados.get("fonte", "desconhecida"))
            .field("status_code", int(status))
            .time(interpretar_timestamp(dados.get("timestamp")), WritePrecision.S)
        )
        for campo in CAMPOS_OBRIGATORIOS + CAMPOS_OPCIONAIS:
            if isinstance(dados.get(campo), (int, float)) and not isinstance(dados.get(campo), bool):
                ponto = ponto.field(campo, float(dados[campo]))
        # Escrita em lote (assíncrona): o ponto entra no buffer e é enviado
        # com os demais; erros de gravação aparecem em erro_ao_gravar.
        write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=ponto)
        log.info("gravado: %s status=%s", dados["machine"], status)

    return ao_receber


def erro_ao_gravar(_conf, _dados, excecao: Exception) -> None:
    log.error("erro ao gravar lote no InfluxDB: %s", excecao)


def principal() -> None:
    influx = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
    # Lotes de até 200 pontos ou a cada 1 s: em operação normal (18 msg/min)
    # cada leitura sai em ~1 s; no backfill do simulador (2000+ mensagens em
    # rajada) a gravação acompanha o broker em vez de estourar a fila dele.
    write_api = influx.write_api(
        write_options=WriteOptions(batch_size=200, flush_interval=1_000, retry_interval=2_000, max_retries=3),
        error_callback=erro_ao_gravar,
    )

    cliente = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="servico-ingestao")
    cliente.on_connect = ao_conectar
    cliente.on_message = criar_callback_mensagem(write_api)
    cliente.reconnect_delay_set(min_delay=1, max_delay=30)
    cliente.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
    log.info("ingestão iniciada, aguardando telemetria")
    try:
        cliente.loop_forever(retry_first_connection=True)
    finally:
        write_api.close()  # descarrega o lote pendente ao encerrar
        influx.close()


if __name__ == "__main__":
    principal()
