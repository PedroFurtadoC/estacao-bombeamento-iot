# Exporta a base de dados para CSV (entrega do Desafio 2)
$raiz = Split-Path -Parent $PSScriptRoot

python -m pip install -q -r "$raiz\backend\tools\requirements.txt"
python "$raiz\backend\tools\exportar_csv.py" --inicio -7d --saida dados/export/telemetria.csv
