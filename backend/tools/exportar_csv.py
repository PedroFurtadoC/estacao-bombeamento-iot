"""Exporta a base de dados (entrega do Desafio 2) do InfluxDB para CSV.

Uso:
    python backend/tools/exportar_csv.py                       # última 24h
    python backend/tools/exportar_csv.py --inicio -7d
    python backend/tools/exportar_csv.py --saida dados/export/telemetria.csv

Lê as credenciais de infra/.env automaticamente (ou das variáveis de ambiente
INFLUX_URL / INFLUX_TOKEN / INFLUX_ORG / INFLUX_BUCKET).
"""

from __future__ import annotations

import argparse
import csv
import os
import sys
from pathlib import Path

from influxdb_client import InfluxDBClient

RAIZ = Path(__file__).resolve().parents[2]

CAMPOS = [
    "temperature",
    "vibration",
    "current",
    "rotation",
    "flow",
    "humidity",
    "gas",
    "status_code",
]


def carregar_env() -> dict:
    """Variáveis de infra/.env (sem sobrescrever o ambiente do processo)."""
    valores: dict[str, str] = {}
    caminho = RAIZ / "infra" / ".env"
    if caminho.exists():
        for linha in caminho.read_text(encoding="utf-8").splitlines():
            linha = linha.strip()
            if linha and not linha.startswith("#") and "=" in linha:
                chave, _, valor = linha.partition("=")
                valores[chave.strip()] = valor.strip()
    valores.update({k: v for k, v in os.environ.items() if k.startswith("INFLUX_")})
    return valores


def principal() -> int:
    parser = argparse.ArgumentParser(description="Exporta a telemetria para CSV")
    parser.add_argument("--inicio", default="-24h", help="início da janela Flux (ex.: -24h, -7d)")
    parser.add_argument("--saida", default="dados/export/telemetria.csv", help="arquivo CSV de saída")
    args = parser.parse_args()

    env = carregar_env()
    token = env.get("INFLUX_TOKEN", "")
    if not token:
        print("ERRO: INFLUX_TOKEN não encontrado (crie infra/.env a partir de infra/.env.example).")
        return 1

    url = env.get("INFLUX_URL", "http://localhost:8086")
    org = env.get("INFLUX_ORG", "unaerp")
    bucket = env.get("INFLUX_BUCKET", "iot")

    consulta = f'''
from(bucket: "{bucket}")
  |> range(start: {args.inicio})
  |> filter(fn: (r) => r._measurement == "telemetria")
  |> pivot(rowKey: ["_time", "machine"], columnKey: ["_field"], valueColumn: "_value")
  |> sort(columns: ["_time"])
'''

    saida = (RAIZ / args.saida) if not Path(args.saida).is_absolute() else Path(args.saida)
    saida.parent.mkdir(parents=True, exist_ok=True)

    linhas = 0
    with InfluxDBClient(url=url, token=token, org=org, timeout=60_000) as cliente:
        tabelas = cliente.query_api().query(consulta)
        with open(saida, "w", newline="", encoding="utf-8") as arquivo:
            escritor = csv.writer(arquivo)
            escritor.writerow(["timestamp", "machine", "fonte", *CAMPOS])
            for tabela in tabelas:
                for registro in tabela.records:
                    v = registro.values
                    escritor.writerow(
                        [
                            registro.get_time().isoformat(),
                            v.get("machine", ""),
                            v.get("fonte", ""),
                            *[v.get(campo, "") for campo in CAMPOS],
                        ]
                    )
                    linhas += 1

    print(f"{linhas} registros exportados para {saida}")
    if linhas >= 500:
        print("Requisito do Desafio 2 atendido (>= 500 registros).")
    else:
        print(f"ATENÇÃO: ainda faltam {500 - linhas} registros para o mínimo de 500.")
    return 0


if __name__ == "__main__":
    sys.exit(principal())
