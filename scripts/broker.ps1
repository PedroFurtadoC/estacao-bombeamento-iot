# Descobre qual IP as ESP32 devem usar para achar o broker MQTT e confere
# se ele esta mesmo respondendo. Rode antes de gravar as placas.
#
# O IP do broker e o IP DESTE notebook na MESMA rede em que as ESP32 estao.
# Ele muda conforme a rede: hotspot do Windows costuma dar 192.168.137.1,
# roteador de casa costuma dar 192.168.0.x ou 192.168.1.x.

$ErrorActionPreference = "SilentlyContinue"
$porta = 1883

function Testar-Porta([string]$ip, [int]$porta) {
    $cli = New-Object System.Net.Sockets.TcpClient
    try {
        $ok = $cli.BeginConnect($ip, $porta, $null, $null).AsyncWaitHandle.WaitOne(1500)
        return ($ok -and $cli.Connected)
    } catch {
        return $false
    } finally {
        $cli.Close()
    }
}

Write-Host ""
Write-Host "Procurando o broker na porta $porta..." -ForegroundColor Cyan
Write-Host ""

$enderecos = Get-NetIPAddress -AddressFamily IPv4 |
    Where-Object {
        $_.IPAddress -ne "127.0.0.1" -and
        -not $_.IPAddress.StartsWith("169.254.")
    } |
    Sort-Object InterfaceAlias

$candidatos = @()

foreach ($e in $enderecos) {
    $ip = $e.IPAddress
    $iface = $e.InterfaceAlias
    $aberta = Testar-Porta $ip $porta

    # Interfaces internas do Docker e do WSL nao servem: a ESP32 nao as enxerga.
    $interna = $iface -match "vEthernet|WSL|Default Switch|VirtualBox|Hyper-V"

    if ($aberta -and -not $interna) {
        $candidatos += [pscustomobject]@{ IP = $ip; Interface = $iface }
        Write-Host ("  {0,-16} {1,-34} SERVE" -f $ip, $iface) -ForegroundColor Green
    } elseif ($aberta) {
        Write-Host ("  {0,-16} {1,-34} interna, nao serve" -f $ip, $iface) -ForegroundColor DarkGray
    } else {
        Write-Host ("  {0,-16} {1,-34} sem resposta" -f $ip, $iface) -ForegroundColor DarkGray
    }
}

Write-Host ""

if ($candidatos.Count -eq 0) {
    Write-Host "Nenhum IP respondeu na porta $porta." -ForegroundColor Red
    Write-Host "A stack esta no ar? Confira com:" -ForegroundColor Yellow
    Write-Host "  docker compose --project-directory infra ps" -ForegroundColor Yellow
    exit 1
}

# Prioriza o hotspot do Windows, depois Wi-Fi, depois o resto.
$escolhido = $candidatos | Where-Object { $_.IP.StartsWith("192.168.137.") } | Select-Object -First 1
if (-not $escolhido) {
    $escolhido = $candidatos | Where-Object { $_.Interface -match "Wi-Fi" } | Select-Object -First 1
}
if (-not $escolhido) {
    $escolhido = $candidatos | Select-Object -First 1
}

Write-Host "Use este IP no firmware/src/secrets.h:" -ForegroundColor Cyan
Write-Host ""
Write-Host ("  #define MQTT_HOST ""{0}""" -f $escolhido.IP) -ForegroundColor Green
Write-Host ("  #define MQTT_PORTA {0}" -f $porta) -ForegroundColor Green
Write-Host ""
Write-Host ("Interface: {0}" -f $escolhido.Interface)
Write-Host "As ESP32 precisam estar nesta mesma rede. Se voce trocar de rede"
Write-Host "ou ligar o hotspot, rode este script de novo: o IP muda."

if ($candidatos.Count -gt 1) {
    Write-Host ""
    Write-Host "Outros IPs que tambem responderam:" -ForegroundColor Yellow
    $candidatos | Where-Object { $_.IP -ne $escolhido.IP } | ForEach-Object {
        Write-Host ("  {0}  ({1})" -f $_.IP, $_.Interface) -ForegroundColor Yellow
    }
    Write-Host "Escolha o da rede em que as placas vao se conectar."
}
Write-Host ""
