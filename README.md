<img width="1077" height="605" alt="image" src="https://github.com/user-attachments/assets/eee428a5-a43f-4586-bf9b-b445a8375a2e" />


Bem-vindo ao repositório da Equipe 01 no DevTITANS! 

Este repositório contém o Hands-On Final da Equipe 01: a implementação de um
**HAL de LoRa** para o Android 16 (produto `devtitans_lora`), indo do
kernel (driver USB) até o app. O sistema permite **enviar e receber mensagens
via rádio LoRa** e **definir uma senha de comunicação** para o dispositivo.

- [Contribuidores](#contribuidores)
- [Recursos](#recursos)
- [Uso](#uso)
- [Contato](#contato)

## [Contribuidores](#contato)
<!--
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
<img src="https://github.com/DevTITANS05/Hands-On-Linux-fork-/assets/21023906/85e61f3e-476c-47a4-82d5-4054e856c67b" width="180" >
-->

<table align="center">
  <tr>
    <td align="center">
      <img src="https://github.com/AlberthVian35069.png" width="100px;" alt="Foto do contribuinte"/>
      <br />
      <b>Alberth Viana</b>
      <br />
      Dev Embarcado
    </td>
    <td align="center">
      <img src="https://github.com/Alleck-Luka.png" width="100px;" alt="Foto do contribuinte"/>
      <br />
      <b>Alleck Luka</b>
      <br />
      Dev AOSP
    </td>
    <td align="center">
      <img src="https://github.com/androvisck.png" width="100px;" alt="Foto do contribuinte"/>
      <br />
      <b>André França</b>
      <br />
      Função
    </td>
    <td align="center">
      <img src="https://github.com/Luiz-Pablo03.png" width="100px;" alt="Foto do contribuinte"/>
      <br />
      <b>Luiz Pablo</b>
      <br />
      Função
    </td>
    <td align="center">
      <img width="100px" alt="Foto do contribuinte" src="https://github.com/user-attachments/assets/e446fc4e-7e2c-4551-ba6d-76993ef12faa" />
      <br />
      <b>Yago Campos</b>
      <br />
      Função
    </td>
  </tr>
</table>

- **Alberth Viana:** Programador Firmware (ESP32)
- **Alleck dos Santos:** Desenvolvedor AOSP
- **André França:** Desenvolvedor AOSP
- **Luiz Pablo:** Portar o Android
- **Yago Campos:** Programador Firmware (ESP32)

## Recursos

- AOSP (Android 16) + emulador Cuttlefish (produto `devtitans_lora`);
- ESP32 + rádio LoRa **E32**;
- Kernel do emulador recompilado (`kernel_recompilado/bzImage`) e o módulo `loracomm.ko`;

## Uso

### Configuração do esp32:

<img width="1370" height="529" alt="image" src="https://github.com/user-attachments/assets/9d9a5192-35cb-413f-9407-db0456abf8d6" />


Você consegue o firmware utilizado no projeto dentro de:
`caos/firmware_lora_rx_tx_aes_get_set_key.ino`
Dependendo da configuração do dispositivo que você utilizar, os pinos podem mudar de posição,
portanto é recomendável que você saiba alterar isso no código acima.
Os periféricos utilizados foram um Esp32 devkit1 e um LoRa Ebyte32.

### Configuração do dispositivo:

Recomendamos iniciar o repositório via `git pull` na raiz do AOSP, assim os arquivos já serão inseridos nos locais corretos.

**IMPORTANTE:** Se fizer isso, pule para a segunda etapa desse tutorial.

#### 1. (OPCIONAL):  Copie o código-fonte para o AOSP:
na raiz do aosp:
```bash
   mkdir kernel_recompilado
   cp -r device/devtitans/lora (RAIZ DO AOSP)/device/devtitans/
   cp -r kernel_recompilado (RAIZ_DO_AOSP)/
```
#### 2. Compile: 
   `cd ~/aosp && source build/envsetup.sh && lunch devtitans_lora && m`
#### 3. Inicie o emulador passando o USB do CP2102 e o kernel:
[Tutorial do emulador](#iniciar-o-emulador-com-passthrough-do-usb)

#### 4. Confira o sysfs: `adb shell ls /sys/kernel/loracomm` (send, ping, aux, rx_count, last_rx, key)
#### 5. Teste pelo cliente binder: `adb shell /vendor/bin/lora_service_client connect|send|ping|get-aux|get-rx-count|get-last-rx|get-key|set-key`
#### 6. Abra o app **LoraApp** ou o **Híperion** no emulador para enviar/receber mensagens e trocar a senha.

Observação: O produto final foi validado em um dispositivo físico G100 (smartphone Motorola de codenome nio) através do projeto LineageOS.

## Iniciar o emulador com Passthrough do USB:

**ESSA PARTE É UM POUCO CONFUSA, MAS GARANTO QUE FUNCIONA -Alleck dos Santos**

Isso leva em consideração que você já escolheu o dispositivo com o lunch devtitans_lora, e que
já compilou o código fonte do Cuttlefish, emulador utilizado nesse projeto.
[Tutorial de instalação do Cuttlefish](#https://source.android.com/docs/devices/cuttlefish/get-started)

### Passo 1:
Criar o emulador com o kernel e initramfs.img, com o usb habilitado:
```bash
cvd create \
  -kernel_path=$ANDROID_BUILD_TOP/kernel_recompilado/bzImage \
  -initramfs_path=$ANDROID_BUILD_TOP/kernel_recompilado/initramfs.img \
  --display=width=1080,height=1920,dpi=440 \
  --enable-usb=true
```

### Passo 2:
Encontrar o socket do emulador:

```bash
find /tmp/cf_avd_* -name "crosvm_control.sock" 2>/dev/null
```
Guarde o output desse comando.

### Passo 3:
Procurar o dispositivo usb:

```bash
lsusb
```
Se for um Esp32, por exemplo, procure por um dispositivo Silicon Labs, e anote o número de Bus e Device. 

### Passo 4:
Inserir o usb via crosvm attach (obs, deve estar na root da aosp):

```bash
sudo ./out/host/linux-x86/bin/crosvm usb attach \
  <BUS>:<DEVICE>:<BUS>:<DEVICE> \
  /dev/bus/usb/<BUS_COM_LEADING_ZEROES>/<DEVICE_COM_LEADING_ZEROES> \
  <SOCKET_DO_EMULADOR>
```
Aqui o socket utilizado é o output do [passo 2](#passo-2).

Exemplo preenchido:

```bash
sudo ./out/host/linux-x86/bin/crosvm usb attach \
  3:14:3:14 \
  /dev/bus/usb/003/014 \
  /tmp/cf_avd_1000/cvd-1/internal/crosvm_control.sock
```

Após isso, você pode verificar o adb shell, e usando dmesg -w, consegue encontrar as mensagens que mostram o dispositivo conectando.

### Utilizando o módulo físico:
O emulador deve ter acesso ao dispositivo através de uma conexão USB - Micro USB (entrada do Esp32).
Já em um smartphone, é necessário, um cabo USB-C (smartphone) -> Micro-USB (Esp32). Conversores também funcionam. 

Um bug conhecido é que dispositivos que já realizaram uma conexão com um Esp32 podem ter baixado o driver de acesso para
desenvolvimento (o CP210x). Se o dispositivo não estiver sendo reconhecido, recomendamos remover este driver temporáriamente:
```bash
   sudo nano /etc/modprobe.d/blacklist.conf
```
Adicione a seguinte linha no final do arquivo:
``
   blacklist cp210x
``

### Validação completa (com o ESP32/LoRa conectados)

<!-- 1. **Preparar o hardware** — ESP32 + rádio E32 ligados e conectados por **cabo de dados** na máquina do emulador; nada segurando a serial (Arduino IDE/`screen`/`picocom` fechados):
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
 -->

Com o dispositivo emulado (ou conectado via adb wireless):
 
1. **Testar direto no sysfs** (comandos do device entre aspas simples, para o redirecionamento acontecer dentro do Android):
```bash
adb shell cat /sys/kernel/loracomm/ping                        # 1 (PONG recebido)
adb shell cat /sys/kernel/loracomm/aux                         # 1 = modo RX / 0 = TX
adb shell 'echo "ola lora" > /sys/kernel/loracomm/send'        # envia pelo rádio
adb shell cat /sys/kernel/loracomm/rx_count                    # nº de mensagens recebidas
adb shell cat /sys/kernel/loracomm/last_rx                     # última mensagem recebida
adb shell 'echo minhasenha > /sys/kernel/loracomm/key'         # SET_KEY
adb shell cat /sys/kernel/loracomm/key                         # minhasenha (GET_KEY)
```

2. **Cliente sysfs (`loracomm_client`)**:
```bash
adb shell /vendor/bin/loracomm_client ping                     # Ping enviado!
adb shell /vendor/bin/loracomm_client get-aux                  # Dispositivo em modo de RX! / ...TX!
adb shell /vendor/bin/loracomm_client get-rx-count             # Quantidade de mensagens recebidas: N
adb shell /vendor/bin/loracomm_client get-last-rx              # Última mensagem: <texto>
adb shell /vendor/bin/loracomm_client send "ola lora"          # Mensagem enviada: ola lora
adb shell /vendor/bin/loracomm_client set-key minhasenha       # Key atualizada: minhasenha
adb shell /vendor/bin/loracomm_client get-key                  # Key atual do dispositivo: minhasenha
```

3. **Cliente binder (`lora_service_client`)** — os 8 métodos do AIDL:
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

4. **Via `service call`** (transaction codes: 1=connect, 2=send, 3=ping, 4=getAux, 5=getRxCount, 6=getLastRx, 7=getKey, 8=setKey):
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

5. **App `LoraApp`**:
- Status deve mostrar **"Conectado"**;
- **Enviar**: digite a mensagem e clique em *Enviar* → toast "Mensagem enviada!";
- **Receber**: com um **segundo nó LoRa** transmitindo, "Mensagens recebidas" e "Última mensagem" atualizam automaticamente (polling de 1 s);
- **Trocar senha**: digite a nova senha e clique em *Trocar senha* → toast "Senha alterada!" e o campo "Key atual" exibe a nova senha.

6. **Checagens finais**:
```bash
adb shell ps -ef | grep -v grep | grep lora-service            # daemon rodando (sem crash)
adb shell ps -Z | grep loraapp                                 # u:r:platform_app:s0
adb shell logcat -d | grep 'avc: denied' | grep -i lora        # deve sair vazio
```
<!--
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

-->

## Contato

- **Alberth Viana** — Programador Firmware (ESP32) [alberthviana3506t@gmail.com](mailto:alberthviana3506t@gmail.com)
- **Alleck Luka** — Desenvolvedor AOSP — [allecklukap@gmail.com](mailto:allecklukap@gmail.com)
- **André França** — Desenvolvedor AOSP — [androvisck@gmail.com](mailto:androvisck@gmail.com)
- **Luiz Pablo** — Portar o Android - [tec.luizpsilva@gmail.com](mailto:tec.luizpsilva@gmail.com)
- **Yago Campos** — Programador Firmware (ESP32) [yagocamposdesouza85@gmail.com](mailto:yagocamposdesouza85@gmail.com)

  
Para perguntas, sugestões ou feedback, entre em contato com o mantenedor do projeto em [allecklukap@gmail.com](mailto:allecklukap@gmail.com).
