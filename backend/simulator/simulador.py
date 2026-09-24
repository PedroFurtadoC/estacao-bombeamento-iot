"""Simulador das 3 máquinas (Desafio 2), e plano B se alguma ESP32 falhar.

Publica no mesmo contrato JSON do firmware, espelhando a montagem de bancada:
as três têm DHT22 (temperatura e umidade), só a M03 tem o MQ de gás. Vibração,
corrente e rotação não têm sensor em nenhuma e saem do modelo.

O ar vem do clima de Ribeirão Preto (seção CLIMA), então a temperatura sobe de
manhã e cai de madrugada, como aconteceria com as placas na casa de bombas.

  Tempo real:   python simulador.py --duracao 300 --intervalo 10
  Backfill:     python simulador.py --backfill 2h --intervalo 10 --anomalia
  Ciclo do dia: python simulador.py --backfill 24h --intervalo 60
"""

from __future__ import annotations

import argparse
import json
import math
import random
import sys
import time
from datetime import datetime, timedelta, timezone

import paho.mqtt.client as mqtt

FUSO_LOCAL = timezone(timedelta(hours=-3))
MAQUINAS = ("M01", "M02", "M03")

# ---------------- CLIMA (Ribeirão Preto, setembro) ----------------
# Mês mais quente do ano na cidade: mínima média de 17 C ao amanhecer e máxima
# de 31 C no meio da tarde, no fim da estação seca.
EXTERNO_MIN = 17.0
EXTERNO_MAX = 31.0
HORA_MIN = 6.0
HORA_MAX = 15.0

# A casa de bombas é fechada, de alvenaria, e abriga os três conjuntos. A
# inércia do prédio corta boa parte da oscilação do ar da rua e o calor dos
# motores segura um piso, então lá dentro varia bem menos do que fora.
AMORTECIMENTO = 0.45  # fração da oscilação externa que chega ao ar de dentro
PISO_MOTORES = 6.0    # C que os motores somam ao ar parado

# Ponto de orvalho do ar seco de setembro. Quase não muda ao longo do dia: é
# ele que derruba a umidade relativa quando a temperatura sobe.
ORVALHO = 11.0

# Espelha TEMP_OFFSET em firmware/src/config.h. Mexeu lá, mexa aqui.
TEMP_OFFSET = 40.0

# Elevação própria de cada conjunto: carga e ventilação não são iguais.
VIES_TERMICO = {"M01": -0.8, "M02": 1.2, "M03": -0.3}

# Inércia térmica da carcaça: persegue o alvo, não salta de um passo ao outro.
INERCIA_TERMICA = 0.25

# ---------------- Sinais sem sensor físico ----------------
# (base, passo, mínimo normal, máximo normal) por sinal
PERFIS = {
    "vibration": (2.2, 0.25, 1.0, 3.4),
    "current": (7.5, 0.3, 6.0, 8.9),
    "rotation": (3500.0, 25.0, 3420.0, 3580.0),
    "flow": (30.0, 1.0, 24.0, 38.0),
}

# Anomalia correlacionada: carga sobe, corrente e temperatura vão juntas,
# vibração acompanha, rotação e vazão caem.
ALVOS_ANOMALIA = {
    "vibration": (5.5, 4.0, 7.0),
    "current": (11.5, 10.0, 13.0),
    "rotation": (3250.0, 3150.0, 3380.0),
    "flow": (12.0, 8.0, 18.0),
}

# Na anomalia a carcaça sobe sobre o clima do momento, com piso e teto, para a
# falha cair na faixa crítica em qualquer hora do dia.
ANOMALIA_SOBRE_CLIMA = 14.0
ANOMALIA_TEMP_MIN = 82.0
ANOMALIA_TEMP_MAX = 88.0

# MQ da M03, em % do fundo de escala do ADC (não é ppm: o sensor não foi
# calibrado com gás de referência). Em ar limpo e aquecido ele marca 31 %,
# medido na bancada, por isso os limiares são 45 e 60.
GAS_BASE = 31.0
GAS_PASSO = 0.8
GAS_MIN = 26.0
GAS_MAX = 38.0
GAS_ANOMALIA = (65.0, 58.0, 72.0)

# Mesmas faixas de firmware/src/config.h e de backend/ingest/main.py.
LIMIARES = {
    #            atenção  crítico
    "temperature": (75.0, 80.0),
    "vibration": (3.5, 5.0),
    "current": (9.0, 11.0),
    "gas": (45.0, 60.0),
}
ROTACAO_ATENCAO = (3400.0, 3600.0)
ROTACAO_CRITICO = (3300.0, 3700.0)
VAZAO_ATENCAO = (20.0, 40.0)
VAZAO_CRITICO = (10.0, 45.0)

# No modo contínuo, mesmos tempos do firmware (SIM_ANOMALIA_* em config.h)
ANOMALIA_PERIODO_S = 15 * 60
ANOMALIA_DURACAO_S = 60


def interpretar_duracao(texto: str) -> timedelta:
    """Converte '2h', '45m' ou '90s' em timedelta."""
    unidades = {"h": 3600, "m": 60, "s": 1}
    if texto and texto[-1] in unidades:
        return timedelta(seconds=float(texto[:-1]) * unidades[texto[-1]])
    return timedelta(seconds=float(texto))


def limitar(valor: float, minimo: float, maximo: float) -> float:
    return max(minimo, min(maximo, valor))


def fracao_do_dia(momento: datetime) -> float:
    """0 na mínima (6h) e 1 na máxima (15h).

    O ar esquenta em 9 horas e leva 15 para esfriar, então são duas
    meias-senoides de duração diferente, não uma senoide simples.
    """
    h = momento.hour + momento.minute / 60 + momento.second / 3600
    if HORA_MIN <= h < HORA_MAX:
        x = (h - HORA_MIN) / (HORA_MAX - HORA_MIN)
        return (1 - math.cos(math.pi * x)) / 2
    x = ((h - HORA_MAX) % 24) / (24 - (HORA_MAX - HORA_MIN))
    return (1 + math.cos(math.pi * x)) / 2


def desvio_do_dia(momento: datetime) -> float:
    """Alguns dias mais quentes que outros, estável dentro do mesmo dia."""
    return math.sin(momento.timetuple().tm_yday * 2.399) * 2.5


def pressao_vapor(t: float) -> float:
    """Pressão de vapor de saturação (hPa) pela fórmula de Magnus."""
    return 6.112 * math.exp((17.625 * t) / (243.04 + t))


def clima(momento: datetime) -> tuple[float, float]:
    """Temperatura (C) e umidade (%) do ar dentro da casa de bombas.

    A umidade sai desse mesmo ar: o ponto de orvalho quase não muda no dia,
    então aquecer o ar sem acrescentar vapor derruba a umidade relativa. É o
    que produz as tardes de 20 a 30 % de setembro.
    """
    media = (EXTERNO_MIN + EXTERNO_MAX) / 2
    desvio = desvio_do_dia(momento)
    externo = EXTERNO_MIN + (EXTERNO_MAX - EXTERNO_MIN) * fracao_do_dia(momento)
    temperatura = media + (externo - media) * AMORTECIMENTO + PISO_MOTORES + desvio
    umidade = 100.0 * pressao_vapor(ORVALHO + desvio / 2) / pressao_vapor(temperatura)
    return temperatura, limitar(umidade, 8.0, 95.0)


def calcular_status(leitura: dict) -> int:
    """0 normal, 1 atenção, 2 crítico. Espelha o firmware e a ingestão."""

    def passou(campo: str, faixa: int) -> bool:
        valor = leitura.get(campo)
        return isinstance(valor, (int, float)) and valor > LIMIARES[campo][faixa]

    def fora(valor: float, faixa: tuple[float, float]) -> bool:
        return not faixa[0] <= valor <= faixa[1]

    rotacao = leitura["rotation"]
    vazao = leitura.get("flow") or 0.0

    if (
        any(passou(campo, 1) for campo in LIMIARES)
        or fora(rotacao, ROTACAO_CRITICO)
        or (vazao > 0 and fora(vazao, VAZAO_CRITICO))
    ):
        return 2
    if (
        any(passou(campo, 0) for campo in LIMIARES)
        or fora(rotacao, ROTACAO_ATENCAO)
        or (vazao > 0 and fora(vazao, VAZAO_ATENCAO))
    ):
        return 1
    return 0


class Maquina:
    """Um conjunto motobomba. tem_gas espelha o MQ, que só a M03 possui."""

    def __init__(self, nome: str) -> None:
        self.nome = nome
        self.valores = {sinal: base for sinal, (base, _, _, _) in PERFIS.items()}
        self.tem_gas = nome == "M03"
        self.gas = GAS_BASE
        self.temperatura: float | None = None  # None = ainda não partiu do clima
        self.vies = VIES_TERMICO[nome]

    def avancar(self, momento: datetime, anomalia: bool) -> dict:
        ar, umidade_ar = clima(momento)
        temperatura = self._avancar_temperatura(ar, anomalia)

        for sinal, (_, passo, minimo, maximo) in PERFIS.items():
            atual = self.valores[sinal] + random.uniform(-passo, passo)
            if anomalia:
                alvo, minimo, maximo = ALVOS_ANOMALIA[sinal]
                atual = atual + (alvo - atual) * 0.35
            self.valores[sinal] = limitar(atual, minimo, maximo)

        leitura = {
            "machine": self.nome,
            "temperature": round(temperatura, 1),
            "vibration": round(self.valores["vibration"], 1),
            "current": round(self.valores["current"], 1),
            "rotation": int(self.valores["rotation"]),
            "flow": round(self.valores["flow"], 1),
            # As três têm DHT22: é o mesmo ar, com diferença pequena conforme
            # a posição do sensor na casa de bombas.
            "humidity": round(limitar(umidade_ar + random.uniform(-2.0, 2.0), 8.0, 95.0), 1),
        }
        if self.tem_gas:
            leitura["gas"] = round(self._avancar_gas(ar, anomalia), 1)

        leitura["status"] = calcular_status(leitura)
        leitura["fonte"] = "simulado"
        return leitura

    def _avancar_temperatura(self, ar: float, anomalia: bool) -> float:
        """Carcaça = ar da casa + TEMP_OFFSET, com inércia e ruído do DHT22."""
        alvo = ar + TEMP_OFFSET + self.vies
        if anomalia:
            alvo = limitar(alvo + ANOMALIA_SOBRE_CLIMA, ANOMALIA_TEMP_MIN, ANOMALIA_TEMP_MAX)
        # Na primeira leitura já parte do clima, sem rampa de partida
        atual = alvo if self.temperatura is None else self.temperatura
        atual += (alvo - atual) * INERCIA_TERMICA
        atual += random.uniform(-0.2, 0.2)  # ruído do DHT22
        self.temperatura = atual
        return atual

    def _avancar_gas(self, ar: float, anomalia: bool) -> float:
        atual = self.gas + random.uniform(-GAS_PASSO, GAS_PASSO)
        if anomalia:
            alvo, minimo, maximo = GAS_ANOMALIA
            self.gas = limitar(atual + (alvo - atual) * 0.35, minimo, maximo)
        else:
            # Ar mais quente volatiliza mais: sobe de leve à tarde
            self.gas = limitar(atual + (ar - 30.0) * 0.05, GAS_MIN, GAS_MAX)
        return self.gas


def em_anomalia(indice: int, total: int, habilitada: bool, maquina: str, intervalo: float) -> bool:
    """Diz se este passo cai na janela de anomalia. Sempre na M01.

    Com total de passos conhecido (backfill, ou --duracao), a janela fica a
    80% do percurso: sobra base normal antes e o pico fica perto do fim, que é
    o que se mostra no zoom. Sem --duracao não existe percurso, então ela se
    repete a cada 15 min, igual ao firmware. Antes devolvia False para sempre
    nesse caso, e o --anomalia do plano B não fazia nada.
    """
    if not habilitada or maquina != "M01":
        return False
    if total > 0:
        inicio = int(total * 0.80)
        fim = inicio + max(6, int(total * 0.05))
        return inicio <= indice < fim
    passos_ciclo = max(2, round(ANOMALIA_PERIODO_S / intervalo))
    passos_janela = max(1, round(ANOMALIA_DURACAO_S / intervalo))
    return indice % passos_ciclo >= passos_ciclo - passos_janela


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
        leitura = maquina.avancar(momento, anomalia)
        leitura["timestamp"] = momento.strftime("%Y-%m-%dT%H:%M:%S")
        topico = f"fabrica/maquinas/{maquina.nome}/telemetria"
        info = cliente.publish(topico, json.dumps(leitura), qos=1)
        info.wait_for_publish(timeout=5)
        publicados += 1
        if leitura["status"] > 0 or publicados % 50 == 0:
            print(
                f"[{publicados:5d}] {momento:%d/%m %H:%M} {leitura['machine']} "
                f"status={leitura['status']} temp={leitura['temperature']} "
                f"umid={leitura['humidity']}"
            )

    if args.backfill:
        janela = interpretar_duracao(args.backfill)
        passos = int(janela.total_seconds() / args.intervalo)
        inicio = datetime.now(FUSO_LOCAL) - janela
        print(f"Backfill: {passos} passos x 3 máquinas = {passos * 3} registros")
        for i in range(passos):
            momento = inicio + timedelta(seconds=i * args.intervalo)
            for maquina in maquinas:
                publicar(maquina, momento, em_anomalia(i, passos, args.anomalia, maquina.nome, args.intervalo))
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
                    publicar(maquina, momento, em_anomalia(i, passos_estimados, args.anomalia, maquina.nome, args.intervalo))
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
