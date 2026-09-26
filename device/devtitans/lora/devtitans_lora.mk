# Herda as configurações do emulador (produto sdk_phone_x86_64)
$(call inherit-product, $(SRC_TARGET_DIR)/product/sdk_phone_x86_64.mk)

# Sobrescreve algumas variáveis com os dados do novo produto
PRODUCT_NAME := devtitans_lora
PRODUCT_DEVICE := lora
PRODUCT_BRAND := LowRanger
PRODUCT_MODEL := LowRangerOne

# devtitans.lora          -> interface AIDL (libs -V1-ndk/-V1-cpp/.jar na imagem)
# devtitans.lora-service  -> daemon do HAL (+ .rc do init e fragmento VINTF)
# lora_service_client     -> cliente do HAL via binder (Lab 8)
# loracomm_lib            -> biblioteca de acesso ao /sys/kernel/loracomm
# loracomm_client         -> cliente direto no sysfs (debug)
PRODUCT_PACKAGES += \
	devtitans.lora \
	devtitans.lora-service \
	loracomm_lib \
	loracomm_client \
	lora_service_client

# Device Framework Matrix: o produto PRECISA do serviço ILora/default
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := device/devtitans/lora/device_framework_matrix.xml
