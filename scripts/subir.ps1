# Sobe a stack local (Mosquitto + InfluxDB + Grafana + ingestao)
$raiz = Split-Path -Parent $PSScriptRoot

if (-not (Test-Path "$raiz\infra\.env")) {
    Copy-Item "$raiz\infra\.env.example" "$raiz\infra\.env"
    Write-Host "infra\.env criado a partir do exemplo - revise as senhas antes de apresentar." -ForegroundColor Yellow
}

docker compose --project-directory "$raiz\infra" up -d --build
docker compose --project-directory "$raiz\infra" ps

Write-Host ""
Write-Host "Grafana:  http://localhost:3000" -ForegroundColor Green
Write-Host "InfluxDB: http://localhost:8086" -ForegroundColor Green
Write-Host "MQTT:     localhost:1883" -ForegroundColor Green
