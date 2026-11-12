# Herda as configurações do emulador (aosp_x86_64_cf_phone)
$(call inherit-product, device/google/cuttlefish/vsoc_x86_64/phone/aosp_cf.mk)

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
# devtitans.loramanager   -> Manager: biblioteca Java de acesso ao HAL (Lab 9)
PRODUCT_PACKAGES += \
	devtitans.lora \
	devtitans.lora-service \
	loracomm_lib \
	loracomm_client \
	lora_service_client \
	LoRaTile \
	lora_device_framework_matrix	#Android.bp com a vintf_compatibility_matrix
	

# ===== MUDANÇA ANDROID 16: framework matrix fica na system_ext agora
# Device Framework Matrix: o produto PRECISA do serviço ILora/default
# DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := device/devtitans/lora/device_framework_matrix.xml

# Manager
PRODUCT_PACKAGES += devtitans.loramanager

# App de teste do Lora (envia/recebe mensagens e troca a senha via HAL) + App hiperion (envia/recebe mensagens via Manager)
PRODUCT_PACKAGES += LoraApp \
	Hiperion

# SubHal do LoRa no Sensors HAL (MultiHAL). Expõe um sensor numérico customizado
# (type = DEVICE_PRIVATE_BASE + 1) que gera uma flag 0 -> 1 a cada mensagem recebida.
# O HalProxy (android.hardware.sensors-service.multihal) carrega este .so via dlopen.
PRODUCT_PACKAGES += lora.sensors.subhal

# Config do MultiHAL no /odm (lido além do /vendor/etc/sensors/hals.conf, que já
# contém o sub-hal do ranchu). Assim não sobrescrevemos o arquivo do emulador.
PRODUCT_COPY_FILES += \
    device/devtitans/lora/sensors/hals.conf:$(TARGET_COPY_OUT_ODM)/etc/sensors/hals.conf
