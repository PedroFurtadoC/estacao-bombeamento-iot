# Extra script do PlatformIO (post) para os environments do LOLIN S2 Mini.
# Troca o esptool.py pelo wrapper tools/esptool_s2.py, que trata o erro de
# porta que o esptool lanca ao reiniciar o S2 pela USB nativa depois de uma
# gravacao ja verificada. Mantem UPLOADERFLAGS e UPLOADCMD da plataforma.
#
# Ativado em platformio.ini:  extra_scripts = post:tools/pio_s2.py
from os.path import join

Import("env")  # noqa: F821 - injetado pelo SCons do PlatformIO

placa = env.BoardConfig()
if placa.get("build.mcu", "") == "esp32s2" and env.subst("$UPLOAD_PROTOCOL") == "esptool":
    plataforma = env.PioPlatform()
    env["ENV"]["ESPTOOLPY_DIR"] = plataforma.get_package_dir("tool-esptoolpy") or ""
    env.Replace(UPLOADER=join(env.subst("$PROJECT_DIR"), "tools", "esptool_s2.py"))
