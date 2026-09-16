"""Simulador das 3 máquinas da fábrica (Desafio 2).

Publica telemetria via MQTT com o mesmo contrato JSON do firmware:
variação normal, pequenas oscilações e janelas de anomalia.

Modos:
  Tempo real:  python simulador.py --duracao 300 --intervalo 10
  Backfill:    python simulador.py --backfill 2h --intervalo 10 --anomalia
               (insere registros retroativos de uma vez, garantindo os 500+)

Também serve de plano B na apresentação, caso alguma ESP32 falhe.
"""

from __future__ import annotations

import argparse
import json
import random
import sys
import time
from datetime import datetime, timedelta, timezone

import paho.mqtt.client as mqtt

FUSO_LOCAL = timezone(timedelta(hours=-3))
MAQUINAS = ("M01", "M02", "M03")

# (base, passo, mínimo normal, máximo normal) por sinal
# Rotação: conjunto motobomba com motor de 2 polos (60 Hz), nominal ~3500 RPM
PERFIS = {
    "temperature": (70.0, 0.6, 65.0, 75.0),
    "vibration": (2.2, 0.25, 1.0, 3.4),
    "current": (7.5, 0.3, 6.0, 8.9),
    "rotation": (3500.0, 25.0, 3420.0, 3580.0),
    "flow": (30.0, 1.0, 24.0, 38.0),
}


def interpretar_duracao(texto: str) -> timedelta:
    """Converte '2h', '45m' ou '90s' em timedelta."""
    unidades = {"h": 3600, "m": 60, "s": 1}
    if texto and texto[-1] in unidades:
        return timedelta(seconds=float(texto[:-1]) * unidades[texto[-1]])
    return timedelta(seconds=float(texto))


def calcular_status(leitura: dict) -> int:
    t, v = leitura["temperature"], leitura["vibration"]
    c, r = leitura["current"], leitura["rotation"]
    q = leitura["flow"]
    if t > 80 or v > 5 or c > 11 or r < 3300 or r > 3700 or q < 10 or q > 45:
        return 2
    if t > 75 or v > 3.5 or c > 9 or r < 3400 or r > 3600 or q < 20 or q > 40:
        return 1
    return 0


class Maquina:
    def __init__(self, nome: str) -> None:
        self.nome = nome
        self.valores = {sinal: base for sinal, (base, _, _, _) in PERFIS.items()}

    def avancar(self, anomalia: bool) -> dict:
        for sinal, (_, passo, minimo, maximo) in PERFIS.items():
            atual = self.valores[sinal] + random.uniform(-passo, passo)
            if anomalia:
                # Anomalia correlacionada: carga sobe -> corrente e temperatura
                # sobem juntas, vibração acompanha, rotação cai.
                alvos = {
                    "temperature": (82.0, 78.0, 86.0),
                    "current": (11.5, 10.0, 13.0),
                    "vibration": (5.5, 4.0, 7.0),
                    "rotation": (3250.0, 3150.0, 3380.0),
                    "flow": (12.0, 8.0, 18.0),
                }
                alvo, minimo, maximo = alvos[sinal]
                atual = atual + (alvo - atual) * 0.35
            self.valores[sinal] = max(minimo, min(maximo, atual))

        leitura = {
            "machine": self.nome,
            "temperature": round(self.valores["temperature"], 1),
            "vibration": round(self.valores["vibration"], 1),
            "current": round(self.valores["current"], 1),
            "rotation": int(self.valores["rotation"]),
            "flow": round(self.valores["flow"], 1),
        }
        leitura["status"] = calcular_status(leitura)
        leitura["fonte"] = "simulado"
        return leitura


def em_anomalia(indice: int, total: int, habilitada: bool, maquina: str) -> bool:
    """Janela de anomalia na M01, em ~80% do percurso, durando ~5% dos passos."""
    if not habilitada or maquina != "M01" or total <= 0:
        return False
    inicio = int(total * 0.80)
    fim = inicio + max(6, int(total * 0.05))
    return inicio <= indice < fim


def principal() -> int:
    parser = argparse.ArgumentParser(description="Simulador das máquinas da fábrica")
    parser.add_argument("--host", default="localhost", help="host do broker MQTT")
    parser.add_argument("--porta", type=int, default=1883, help="porta do broker")
    parser.add_argument("--intervalo", type=float, default=10.0, help="segundos entre leituras")
    parser.add_argument("--duracao", type=float, default=0, help="segundos de execução em tempo real (0 = infinito)")
    parser.add_argument("--backfill", help="gera registros retroativos, ex.: 2h, 45m")
    parser.add_argument("--anomalia", action="store_true", help="injeta janela de anomalia na M01")
    args = parser.parse_args()

    cliente = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="simulador-fabrica")
    cliente.connect(args.host, args.porta, keepalive=60)
    cliente.loop_start()

    maquinas = [Maquina(nome) for nome in MAQUINAS]
    publicados = 0

    def publicar(maquina: Maquina, momento: datetime, anomalia: bool) -> None:
        nonlocal publicados
        leitura = maquina.avancar(anomalia)
        leitura["timestamp"] = momento.strftime("%Y-%m-%dT%H:%M:%S")
        topico = f"fabrica/maquinas/{maquina.nome}/telemetria"
        info = cliente.publish(topico, json.dumps(leitura), qos=1)
        info.wait_for_publish(timeout=5)
        publicados += 1
        if leitura["status"] > 0 or publicados % 50 == 0:
            print(f"[{publicados:5d}] {leitura['machine']} status={leitura['status']} temp={leitura['temperature']}")

    if args.backfill:
        janela = interpretar_duracao(args.backfill)
        passos = int(janela.total_seconds() / args.intervalo)
        inicio = datetime.now(FUSO_LOCAL) - janela
        print(f"Backfill: {passos} passos x 3 máquinas = {passos * 3} registros")
        for i in range(passos):
            momento = inicio + timedelta(seconds=i * args.intervalo)
            for maquina in maquinas:
                publicar(maquina, momento, em_anomalia(i, passos, args.anomalia, maquina.nome))
        print(f"Concluído: {publicados} registros publicados.")
    else:
        fim = time.monotonic() + args.duracao if args.duracao else None
        passos_estimados = int(args.duracao / args.intervalo) if args.duracao else 0
        i = 0
        print("Tempo real. Ctrl+C para encerrar")
        try:
            while fim is None or time.monotonic() < fim:
                momento = datetime.now(FUSO_LOCAL)
                for maquina in maquinas:
                    publicar(maquina, momento, em_anomalia(i, passos_estimados, args.anomalia, maquina.nome))
                i += 1
                time.sleep(args.intervalo)
        except KeyboardInterrupt:
            pass
        print(f"\nEncerrado: {publicados} registros publicados.")

    cliente.loop_stop()
    cliente.disconnect()
    return 0


if __name__ == "__main__":
    sys.exit(principal())
