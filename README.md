# DevTITANS 09 - HandsOn Final - Equipe 01

Bem-vindo ao repositório da Equipe 01 no DevTITANS! 

Este repositório contém o Hands-On Final da Equipe 01: a implementação de um
**HAL de LoRa** para o Android (AOSP 14, produto `devtitans_lora`), indo do
kernel (driver USB) até o app. O sistema permite **enviar e receber mensagens
via rádio LoRa** e **trocar a senha (key)** do dispositivo, passando por driver
Linux, biblioteca C++, HAL AIDL, Manager Java e firmware ESP32.

- [Contribuidores](#contribuidores)
- [Recursos](#recursos)
- [Uso](#uso)
- [Contato](#contato)

## Contribuidores

<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >

- **Alberth Viana:** Programador Firmware (ESP32)
- **Alleck dos Santos:** Desenvolvedor AOSP
- **André França:** Desenvolvedor AOSP
- **Leandro Henrique:** Portar o Android
- **Luiz Pablo:** Portar o Android
- **Yago Campos:** Programador Firmware (ESP32)

## Recursos

- AOSP (Android 14) + emulador `x86_64` (produto `devtitans_lora-eng`);
- ESP32 + rádio LoRa **E32** + chip USB **CP2102** (`10c4:ea60`);
- Kernel do emulador recompilado (`kernel_recompilado/bzImage`) e o módulo `loracomm.ko`;
- (Opcional) um segundo nó LoRa para validar a troca de mensagens.

Board Figma: https://www.figma.com/board/xq28UoX4sbdm6674I8hIIa/Hands-on-DevTITANS?node-id=0-1&t=7G43BBj8pZBM9fgl-1

## Uso

1. Copie o device tree para o AOSP:
   `cp -r device/devtitans/lora ~/aosp/device/devtitans/`
2. Compile: `cd ~/aosp && source build/envsetup.sh && lunch devtitans_lora-eng && m`
3. Inicie o emulador passando o USB do CP2102:
   `emulator -qemu -device qemu-xhci,id=xhci -device usb-host,bus=xhci.0,vendorid=0x10c4,productid=0xea60 &`
4. Carregue o módulo: `adb root && adb push kernel_recompilado/loracomm.ko /data/local/tmp/ && adb shell insmod /data/local/tmp/loracomm.ko`
5. Confira o sysfs: `adb shell ls /sys/kernel/loracomm` (send, ping, aux, rx_count, last_rx, key)
6. Teste pelo cliente binder: `adb shell /vendor/bin/lora_service_client connect|send|ping|get-aux|get-rx-count|get-last-rx|get-key|set-key`
7. Abra o app **LoraApp** no emulador para enviar/receber mensagens e trocar a senha.

### Validação completa (com o ESP32/LoRa conectados)

1. **Preparar o hardware** — ESP32 + rádio E32 ligados e conectados por **cabo de dados** na máquina do emulador; nada segurando a serial (Arduino IDE/`screen`/`picocom` fechados):
```bash
lsusb | grep -i 10c4          # deve mostrar o CP2102 (10c4:ea60)
ls -l /dev/ttyUSB*
```

2. **Subir o emulador com o USB passthrough** (tem que ser no boot do emulador):
```bash
emulator -qemu -device qemu-xhci,id=xhci -device usb-host,bus=xhci.0,vendorid=0x10c4,productid=0xea60 &
# o dispositivo precisa aparecer no barramento USB do emulador:
adb shell 'for f in /sys/bus/usb/devices/*/idVendor; do echo "$f $(cat $f)"; done' | grep -i 10c4
```

3. **Carregar o módulo e conferir o probe**:
```bash
adb root
adb push kernel_recompilado/loracomm.ko /data/local/tmp/
adb shell insmod /data/local/tmp/loracomm.ko
adb shell dmesg | grep -i loracomm     # espera: "LoRaComm: === DISPOSITIVO CONECTADO ==="
adb shell ls /sys/kernel/loracomm      # espera: aux key last_rx ping rx_count send
```
> O diretório `/sys/kernel/loracomm` só é criado no `usb_probe`. Se você deu `insmod` antes do device aparecer no USB, rode `adb shell rmmod loracomm && adb shell insmod /data/local/tmp/loracomm.ko`.

4. **Testar direto no sysfs** (comandos do device entre aspas simples, para o redirecionamento acontecer dentro do Android):
```bash
adb shell cat /sys/kernel/loracomm/ping                        # 1 (PONG recebido)
adb shell cat /sys/kernel/loracomm/aux                         # 1 = modo RX / 0 = TX
adb shell 'echo "ola lora" > /sys/kernel/loracomm/send'        # envia pelo rádio
adb shell cat /sys/kernel/loracomm/rx_count                    # nº de mensagens recebidas
adb shell cat /sys/kernel/loracomm/last_rx                     # última mensagem recebida
adb shell 'echo minhasenha > /sys/kernel/loracomm/key'         # SET_KEY
adb shell cat /sys/kernel/loracomm/key                         # minhasenha (GET_KEY)
```

5. **Cliente sysfs (`loracomm_client`)**:
```bash
adb shell /vendor/bin/loracomm_client ping                     # Ping enviado!
adb shell /vendor/bin/loracomm_client get-aux                  # Dispositivo em modo de RX! / ...TX!
adb shell /vendor/bin/loracomm_client get-rx-count             # Quantidade de mensagens recebidas: N
adb shell /vendor/bin/loracomm_client get-last-rx              # Última mensagem: <texto>
adb shell /vendor/bin/loracomm_client send "ola lora"          # Mensagem enviada: ola lora
adb shell /vendor/bin/loracomm_client set-key minhasenha       # Key atualizada: minhasenha
adb shell /vendor/bin/loracomm_client get-key                  # Key atual do dispositivo: minhasenha
```

6. **Cliente binder (`lora_service_client`)** — os 8 métodos do AIDL:
```bash
adb shell /vendor/bin/lora_service_client connect              # Dispositivo conectado!
adb shell /vendor/bin/lora_service_client ping                 # Ping enviado!
adb shell /vendor/bin/lora_service_client get-aux              # Dispositivo em modo de RX!
adb shell /vendor/bin/lora_service_client get-rx-count         # Quantidade de mensagens recebidas: N
adb shell /vendor/bin/lora_service_client get-last-rx          # Última mensagem: <texto>
adb shell /vendor/bin/lora_service_client send "ola lora"      # Mensagem enviada: ola lora
adb shell /vendor/bin/lora_service_client get-key              # Key atual do dispositivo: minhasenha
adb shell /vendor/bin/lora_service_client set-key novaSenha    # Key atualizada: novaSenha
adb shell ps -ef | grep -v grep | grep lora-service            # daemon continua rodando
```

7. **Via `service call`** (transaction codes: 1=connect, 2=send, 3=ping, 4=getAux, 5=getRxCount, 6=getLastRx, 7=getKey, 8=setKey):
```bash
adb shell service call devtitans.lora.ILora/default 1                  # connect    → 00000001
adb shell service call devtitans.lora.ILora/default 2 s16 "ola"        # send       → 00000001
adb shell service call devtitans.lora.ILora/default 3                  # ping       → 00000001
adb shell service call devtitans.lora.ILora/default 4                  # getAux     → 0/1
adb shell service call devtitans.lora.ILora/default 5                  # getRxCount
adb shell service call devtitans.lora.ILora/default 6                  # getLastRx
adb shell service call devtitans.lora.ILora/default 7                  # getKey
adb shell service call devtitans.lora.ILora/default 8 s16 "x"          # setKey
```

8. **App `LoraApp`** (no emulador):
- Status deve mostrar **"Conectado"**;
- **Enviar**: digite a mensagem e clique em *Enviar* → toast "Mensagem enviada!";
- **Receber**: com um **segundo nó LoRa** transmitindo, "Mensagens recebidas" e "Última mensagem" atualizam automaticamente (polling de 1 s);
- **Trocar senha**: digite a nova senha e clique em *Trocar senha* → toast "Senha alterada!" e o campo "Key atual" exibe a nova senha.

9. **Checagens finais**:
```bash
adb shell ps -ef | grep -v grep | grep lora-service            # daemon rodando (sem crash)
adb shell ps -Z | grep loraapp                                 # u:r:platform_app:s0
adb shell logcat -d | grep 'avc: denied' | grep -i lora        # deve sair vazio
```

> Limitação conhecida: a `key` fica apenas em memória no ESP32 — o `GET_KEY` retorna o valor após o `SET_KEY`, mas ela volta a ser vazia se o ESP32 for reiniciado (o firmware atual não grava a senha em flash).

### Validação parcial (sem o dispositivo)

**1. Build**
```bash
cd ~/aosp
source build/envsetup.sh
lunch devtitans_lora-eng
m                     # compila HAL, clientes, Manager e LoraApp
```

**2. HAL no binder**
```bash
emulator -qemu &      # (sem o usb-host do CP2102)
adb shell ps -ef | grep -v grep | grep lora-service     # deve aparecer o processo
adb shell service list | grep ILora                     # devtitans.lora.ILora/default
adb shell ls -lhZ /vendor/bin/hw/devtitans.lora-service # u:object_r:lora_daemon_exec:s0
adb shell cat /system/etc/vintf/compatibility_matrix.device.xml | grep -A3 -i lora
```

**3. Cliente binder (todos os métodos, esperando "offline")**
```bash
adb shell /vendor/bin/lora_service_client connect # "Dispositivo não encontrado!"
adb shell /vendor/bin/lora_service_client ping # "Erro ao enviar ping" (false)
adb shell /vendor/bin/lora_service_client get-aux # "Dispositivo em TX!" (0)
adb shell /vendor/bin/lora_service_client get-rx-count # "0"
adb shell /vendor/bin/lora_service_client get-last-rx # "" (vazio)
adb shell /vendor/bin/lora_service_client get-key # "" (vazio)
adb shell /vendor/bin/lora_service_client set-key "x"
adb shell ps -ef | grep -v grep | grep lora-service
```

**4. Via `service call` (sem recompilar nada além do AOSP)**
```bash
adb shell service call devtitans.lora.ILora/default 7          # getKey  → Parcel (vazio)
adb shell service call devtitans.lora.ILora/default 8 s16 "x"  # setKey  → Parcel 00000000 (false)
adb shell service call devtitans.lora.ILora/default 1          # connect → Parcel 00000000 (0)
```

**5. Manager + App**
```bash
adb shell ls -lh /system_ext/framework/devtitans.loramanager.jar
adb shell ls -lh /system_ext/app/LoraApp/LoraApp.apk
# abrir o LoraApp: status "Desconectado", botões não crasham, toasts de erro ok
adb shell ps -Z | grep loraapp          # u:r:platform_app:s0
```

**6. SELinux**
```bash
adb shell logcat -d | grep 'avc: denied' | grep -i lora   # não deve aparecer nada
```

## Contato

- **Alberth Viana** — Programador Firmware (ESP32)
- **Alleck dos Santos** — Desenvolvedor AOSP — [allecklukap@gmail.com](mailto:allecklukap@gmail.com)
- **André França** — Desenvolvedor AOSP — [androvisck@gmail.com](mailto:androvisck@gmail.com)
- **Leandro Henrique** — Portar o Android
- **Luiz Pablo** — Portar o Android
- **Yago Campos** — Programador Firmware (ESP32)
<!-- Para perguntas, sugestões ou feedback, entre em contato com o mantenedor do projeto em [maintainer@example.com](mailto:maintainer@example.com). -->
