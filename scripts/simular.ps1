# Gera os 500+ registros do Desafio 2 (backfill de 2h com anomalia na M01)
# e segue publicando em tempo real ate Ctrl+C.
$raiz = Split-Path -Parent $PSScriptRoot

python -m pip install -q -r "$raiz\backend\simulator\requirements.txt"
python "$raiz\backend\simulator\simulador.py" --backfill 2h --intervalo 10 --anomalia
python "$raiz\backend\simulator\simulador.py" --intervalo 10
