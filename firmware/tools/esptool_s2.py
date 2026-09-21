"""Wrapper do esptool para o LOLIN S2 Mini (USB nativa / USB-OTG).

Problema: depois de gravar, o esptool reinicia o S2 por watchdog (ou por
DTR/RTS). O chip some da USB antes de o esptool terminar de falar com a
porta, o pySerial lanca "Cannot configure port" e o PlatformIO marca a
gravacao como FAILED - mesmo com "Hash of data verified" impresso. Isso ainda
derruba o "Upload and Monitor" do VS Code, porque o monitor nao abre apos um
codigo de saida diferente de zero.

Este wrapper e chamado no lugar do esptool.py (ver pio_s2.py). Ele repassa
os argumentos ao esptool real, ecoa a saida em tempo real e:
  * se o esptool terminar sem erro, devolve o mesmo codigo;
  * se falhar SOMENTE depois de imprimir "Hard resetting ..." (a gravacao ja
    foi verificada nesse ponto), espera a placa reaparecer com a serial do
    firmware e devolve 0;
  * qualquer outro erro continua sendo erro.
"""

import os
import subprocess
import sys
import time

MARCAS_DE_RESET = ("Hard resetting with a watchdog", "Hard resetting via RTS pin")
PID_ROM_S2 = "303A:0002"  # bootloader ROM do ESP32-S2 em modo de gravacao
TEMPO_MAX_REENUMERACAO_S = 12


def listar_portas():
    try:
        from serial.tools import list_ports
    except ImportError:
        return []
    return [(p.device, (p.hwid or "").upper()) for p in list_ports.comports()]


def esperar_porta_do_firmware(porta_rom):
    """Aguarda a porta do ROM sumir e uma porta do firmware (nao-ROM) aparecer."""
    limite = time.time() + TEMPO_MAX_REENUMERACAO_S
    while time.time() < limite:
        portas = listar_portas()
        rom_presente = any(dev == porta_rom or PID_ROM_S2 in hw for dev, hw in portas)
        app = [dev for dev, hw in portas if "303A:" in hw and PID_ROM_S2 not in hw]
        if app and not rom_presente:
            return app[0]
        time.sleep(0.25)
    return None


def main():
    esptool_dir = os.environ.get("ESPTOOLPY_DIR")
    if not esptool_dir:
        sys.stderr.write("esptool_s2.py: variavel ESPTOOLPY_DIR ausente (ver tools/pio_s2.py)\n")
        return 2

    args = sys.argv[1:]
    porta = None
    if "--port" in args:
        porta = args[args.index("--port") + 1]

    cmd = [sys.executable, os.path.join(esptool_dir, "esptool.py")] + args
    proc = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1
    )
    viu_reset = False
    for linha in proc.stdout:
        sys.stdout.write(linha)
        sys.stdout.flush()
        if any(marca in linha for marca in MARCAS_DE_RESET):
            viu_reset = True
    rc = proc.wait()

    if rc == 0 or not viu_reset:
        return rc

    print(
        "\n[esptool_s2] Gravacao verificada. O erro de porta acima e esperado no "
        "S2 pela USB nativa: a placa reiniciou antes de o esptool fechar a porta."
    )
    nova = esperar_porta_do_firmware(porta)
    if nova:
        print(f"[esptool_s2] Placa de volta com a serial do firmware em {nova}.")
    else:
        print(
            "[esptool_s2] A placa nao reapareceu sozinha; se necessario, aperte RST. "
            "A gravacao esta feita."
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
