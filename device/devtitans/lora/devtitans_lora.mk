# Herda as configurações do emulador (produto sdk_phone_x86_64)
$(call inherit-product, $(SRC_TARGET_DIR)/product/sdk_phone_x86_64.mk)

# Sobrescreve algumas variáveis com os dados do novo produto
PRODUCT_NAME := devtitans_lora
PRODUCT_DEVICE := lora
PRODUCT_BRAND := LowRanger
PRODUCT_MODEL := LowRangerOne

PRODUCT_PACKAGES += \
	lora_service_client \
	loracomm_lib \
	loracomm_client
