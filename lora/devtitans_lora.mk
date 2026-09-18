# Herda as configurações do emulador (produto sdk_phone_x86_64)
$(call inherit-product, $(SRC_TARGET_DIR)/product/sdk_phone_x86_64.mk)

# Sobrescreve algumas variáveis com os dados do novo produto
PRODUCT_NAME := devtitans_lora
PRODUCT_DEVICE := lora
PRODUCT_BRAND := LoraBrand
PRODUCT_MODEL := LoraModel

# Lora AIDL Interface & Service
PRODUCT_PACKAGES += devtitans.lora
PRODUCT_PACKAGES += devtitans.lora-service

# Device Framework Matrix
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := device/devtitans/lora/device_framework_matrix.xml

# SELinux Policies Directory
BOARD_SEPOLICY_DIRS += device/devtitans/lora/sepolicy