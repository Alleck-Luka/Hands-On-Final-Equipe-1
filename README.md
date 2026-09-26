# DevTITANS 09 - HandsOn Final - Equipe 01

Bem-vindo ao repositório da Equipe 01 no DevTITANS! 

{ TODO: Descrição do projeto }

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
- **André França:** androvisck@gmail.com
- **Leandro Henrique:** Portar o Android
- **Luiz Pablo:** Portar o Android
- **Yago Campos:** Programador Firmware (ESP32)

## Recursos

{ TODO: Listar os recursos necessários como sensores, dispositivos, etc.}<br/>
Board Figma: https://www.figma.com/board/xq28UoX4sbdm6674I8hIIa/Hands-on-DevTITANS?node-id=0-1&t=7G43BBj8pZBM9fgl-1

## Uso


## A validação parcial

Sem o device, `Loracomm::connect()` retorna 0 e `readFileValue()` retorna `""`. Mas 3 métodos fazem `stoi/stol("")`, o que **lança exceção e derruba o processo**:

```cpp
// loracomm_lib.cpp
bool Loracomm::ping()      { return stoi(this->readFileValue("ping")); }       // crash offline
bool Loracomm::getAux()    { return stoi(this->readFileValue("aux")); }        // crash offline
long Loracomm::getRxCount(){ return stol(this->readFileValue("rx_count")); }   // crash offline
```

Consequência prática: o `LoraApp` faz polling de `getRxCount()` a cada 1 s → **sem ESP32 o daemon `devtitans.lora-service` cai em ~1s**. Então, para validar parcialmente, precisamos corrigir isso primeiro.


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
adb shell 

ps -ef | grep -v grep | grep lora-service     # deve aparecer o processo
service list | grep ILora # deve listar devtitans.lora.ILora/default
ls -lhZ /vendor/bin/hw/devtitans.lora-service # u:object_r:lora_daemon_exec:s0
cat /system/etc/vintf/compatibility_matrix.device.xml | grep -A3 -i lora
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

**6. SELinux limpo**
```bash
adb shell logcat -d | grep 'avc: denied' | grep -i lora   # não deve aparecer nada
```

## O que NÃO dá para validar sem o hardware
- `connect()` retornando 1, `get-aux` = 1, `/sys/kernel/loracomm/*`, envio/recepção reais via rádio, e a **persistência da senha no ESP32** (o `SET_KEY` de verdade). Esses só com o ESP32/LoRa plugado + `usb-host`.

Estou em modo Plan (não editei nada). **Alterne para o modo Act** que eu aplico o Fix 1 (essencial) e o Fix 2 (CLI `get-key`/`set-key`), e te deixo o `loracomm_lib.cpp` e o `lora_service_client.cpp` prontos para essa validação parcial.


## Contato

{ TODO: Adicionar contato }
<!-- Para perguntas, sugestões ou feedback, entre em contato com o mantenedor do projeto em [maintainer@example.com](mailto:maintainer@example.com). -->
