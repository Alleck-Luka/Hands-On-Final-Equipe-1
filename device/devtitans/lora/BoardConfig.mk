# Inclui as configurações do "hardware" do emulador
include build/make/target/board/emulator_x86_64/BoardConfig.mk

# SELinux do Lora: registra sepolicy/ (lora.te, file_contexts, service_contexts e
# property_contexts) para o build concatenar na policy vendor do device.
# Sem isso o init não faz a transição para lora_daemon e o HAL não sobe.
BOARD_VENDOR_SEPOLICY_DIRS += device/devtitans/lora/sepolicy
