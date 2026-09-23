# 09 - Guia de Bancada

Roteiro direto para montar, gravar e apresentar. Tudo roda **local**, no
notebook: nenhum serviço na nuvem, nenhuma dependência de internet.

## O que cada máquina leva

| Máquina | Placa | Sensores | GPIOs | Atuador |
|---|---|---|---|---|
| M01 | DevKit | DHT22 + vazão YF-S201C | 4 (DHT), 33 (vazão) | LED RGB HW-479 (25, 26, 27) |
| M02 | ESP32-S3-N16R8 | DHT22 + vazão YF-S402 | 4 (DHT), 5 (vazão) | nenhum |
| M03 | DevKit | DHT22 + MQ (gás) | 4 (DHT), 34 (MQ) | LED flash HW-481 (25) |

As duas DevKit (M01 e M03) são iguais: o 5 V é o pino `VIN` e o `GPIO 4` do
DHT22 fica na fileira oposta ao `VIN`. Ao gravar, se o upload travar em
`Connecting...`, segure o `BOOT` até começar. O S3 grava pela porta `UART`
sem botão. Etiquete as duas DevKit logo depois de gravar: por fora são
idênticas e só o firmware diz qual é a M01 e qual é a M03.

Esquema elétrico completo, pino a pino, e a **lista do que levar para a
bancada**: [`10-esquema-eletrico.md`](10-esquema-eletrico.md).

Regra rápida: **DHT22 em 3,3 V**, ligação direta no GPIO. **MQ (M03) e
vazão da M01 em 5 V**, com divisor de 10 kΩ e 20 kΩ antes do GPIO; a **vazão
da M02 fica em 3,3 V, direto no GPIO 5**, sem divisor (pull-up interno, já no
firmware). O sensor de vazão em 5 V ainda precisa de pull-up, e o esquema
explica como descobrir se o seu modelo já tem um interno.

## Preparação (uma vez)

```powershell
# 1. Subir a stack local
.\scripts\subir.ps1

# 2. Conferir que tudo está no ar
docker compose --project-directory infra ps
```

Grafana em <http://localhost:3000>, usuário e senha no `infra/.env`.

## Dia da montagem

### 1. Ligar o hotspot do notebook

Windows: Configurações, Rede e Internet, Ponto de acesso móvel. Ligar e anotar
o nome e a senha.

### 2. Descobrir o IP do notebook na rede do hotspot

```powershell
ipconfig
```

Procure o adaptador do ponto de acesso móvel. O IPv4 costuma ser
`192.168.137.1`. Esse é o endereço do broker para as ESP32.

### 3. Preencher o secrets.h

```powershell
Copy-Item firmware/src/secrets.h.example firmware/src/secrets.h
```

Editar o arquivo com o nome do hotspot, a senha e o IP do passo 2.

### 4. Gravar as três ESP32

Conectar uma por vez e rodar o environment correspondente:

```powershell
pio run -d firmware -e maquina01 -t upload
pio run -d firmware -e maquina02 -t upload
pio run -d firmware -e maquina03 -t upload
```

Dica: marque cada placa com uma etiqueta (M01, M02, M03) logo após gravar.

### 5. Conferir que os dados estão chegando

Três verificações, do dispositivo até o dashboard:

```powershell
# a) A ESP32 está lendo e publicando
pio device monitor -b 115200

# b) O back-end está gravando
docker compose --project-directory infra logs -f ingestao

# c) O dashboard está mostrando
# abrir http://localhost:3000
```

No monitor serial você deve ver o Wi-Fi conectar, o MQTT conectar e uma linha
`[pub]` com o JSON a cada 10 segundos. Nos logs da ingestão, uma linha
`gravado: M01 status=0` por leitura.

## Roteiro da apresentação

1. Stack no ar e as 3 ESP32 ligadas, publicando (deixar rodando alguns minutos
   antes para o dashboard já ter histórico);
2. Mostrar o dashboard completo com as 3 máquinas em condição normal;
3. **Provocar a anomalia ao vivo**: segurar o sensor DHT22 entre os dedos. Em
   poucos segundos a temperatura passa de 80 °C, o LED vira vermelho e os
   painéis de alerta reagem;
4. Outras provocações possíveis: aproximar gás de isqueiro sem acender ou
   álcool do MQ (gás, M03), soprar na turbina do sensor de vazão (M01 e M02);
5. Mostrar o histórico e a análise (Desafio 4).

## Se algo der errado

| Sintoma | Causa provável | O que fazer |
|---|---|---|
| Serial trava em `[wifi] conectando...` | Nome ou senha do hotspot errados, ou hotspot em 5 GHz | Conferir o `secrets.h`; o ESP32 só enxerga redes de 2,4 GHz |
| `[mqtt] falhou (rc=-2)` | IP do broker errado ou firewall do Windows | Conferir o IP com `ipconfig`; liberar a porta 1883 no firewall |
| Dashboard vazio | Ingestão parada ou sem dados | `docker compose --project-directory infra logs ingestao` |
| Temperatura marcando `nan` | DHT22 mal ligado ou no pino errado | Conferir VCC em 3,3 V e o dado no GPIO 4 |
| Vazão sempre em 0 | Bancada seca (esperado), divisor mal montado ou falta de pull-up | Soprar na turbina; se continuar em 0, conferir o divisor e o pull-up no esquema elétrico |
| Gás sempre alto ou saturado | MQ ainda aquecendo | Aguardar cerca de 2 min após ligar |
| ESP32 reiniciando sozinha | Alimentação fraca na M03 | Usar carregador de 5 V com 1 A ou mais |

## Plano B (garantido)

Se alguma ESP32 falhar no dia, o simulador publica as 3 máquinas e a
apresentação continua sem interrupção:

```powershell
python backend/simulator/simulador.py --intervalo 10 --anomalia
```

Leve também uma ESP32 reserva já gravada.
