#include "LoraSubHal.h"

#include "loracomm_lib.h"

#include <unistd.h>              // usleep
#include <utils/SystemClock.h>   // ::android::elapsedRealtimeNano()

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

using ::devtitans::loracomm::Loracomm;

namespace {
// Sensor customizado: DEVICE_PRIVATE_BASE (0x10000) + 1 = 65537.
// O pacote 1.0 é a origem de SensorType; o cast evita depender do tipo de
// enum (scoped/unscoped) gerado pelo HIDL.
constexpr int32_t kLoraSensorType = static_cast<int32_t>(SensorType::DEVICE_PRIVATE_BASE) + 1;
constexpr int32_t kLoraSensorHandle = 1;
}  // namespace

Return<Result> LoraSubHal::initialize(const sp<IHalProxyCallback>& cb) {
    mCallback = cb;
    return Result::OK;
}

Return<void> LoraSubHal::getSensorsList(ISensors::getSensorsList_cb _hidl_cb) {
    SensorInfo info = {};
    info.name                   = "LoRa";
    info.vendor                 = "DevTITANS";
    info.version                = 1;
    info.sensorHandle           = kLoraSensorHandle;
    info.type                   = static_cast<SensorType>(kLoraSensorType);
    info.typeAsString           = "devtitans.lora";   // obrigatório para tipo custom
    info.maxRange               = 1.0f;
    info.resolution             = 1.0f;
    info.power                  = 0.1f;
    info.minDelay               = 500000;    // 500 ms (em µs)
    info.fifoReservedEventCount = 0;
    info.fifoMaxEventCount      = 0;
    info.requiredPermission     = "";
    info.maxDelay               = 1000000;   // 1 s (em µs)
    info.flags                  = 0;         // contínuo: entrega todo evento postado

    _hidl_cb({ info });
    return Void();
}

Return<Result> LoraSubHal::activate(int32_t /*sensorHandle*/, bool enabled) {
    if (enabled && !mActive.exchange(true)) {
        mThread = std::thread(&LoraSubHal::pollLoop, this);
    } else if (!enabled) {
        mActive = false;
        if (mThread.joinable()) {
            mThread.join();
        }
    }
    return Result::OK;
}

void LoraSubHal::pollLoop() {
    Loracomm lora;

    while (mActive) {
        long count = lora.getRxCount();      // lê /sys/kernel/loracomm/rx_count

        if (count != mLastCount) {           // mensagem nova chegou
            mLastCount = count;
            postEvent(1.0f);                 // flag 0 -> 1
            mPulseActive = true;
        } else if (mPulseActive) {           // ciclo seguinte sem novidade
            postEvent(0.0f);                 // flag 1 -> 0 (reset)
            mPulseActive = false;
        }

        usleep(500000);                      // 500 ms
    }
}

void LoraSubHal::postEvent(float value) {
    // Event zero-inicializado (agregado HIDL) para não vazar lixo em data[1..15].
    Event e = {};
    e.timestamp    = ::android::elapsedRealtimeNano();
    e.sensorHandle = kLoraSensorHandle;
    e.sensorType   = static_cast<SensorType>(kLoraSensorType);

    // Para tipos custom (>= DEVICE_PRIVATE_BASE) o framework copia u.data (float[16])
    // para sensors_event_t.data -> chega no app em event.values[].
    // u.data é um hidl_array<float,16>; .data() devolve o float* (padrão usado no
    // próprio AOSP em sensors/common/1.0/default/convert.cpp: src.u.data.data()).
    e.u.data.data()[0] = value;

    // O HalProxy faz o push real para o framework (entrega ao onSensorChanged).
    // O wakelock fica desbloqueado pois não é um sensor wake-up.
    mCallback->postEvents({ e }, mCallback->createScopedWakelock(false));
}

Return<void> LoraSubHal::registerDirectChannel(const SharedMemInfo&,
                                               ISensors::registerDirectChannel_cb _hidl_cb) {
    _hidl_cb(Result::INVALID_OPERATION, -1);   // direct channel não suportado
    return Void();
}

Return<void> LoraSubHal::configDirectReport(int32_t, int32_t, RateLevel,
                                            ISensors::configDirectReport_cb _hidl_cb) {
    _hidl_cb(Result::INVALID_OPERATION, -1);   // direct report não suportado
    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android

// Função exportada obrigatória: o HalProxy faz dlopen deste .so e chama
// sensorsHalGetSubHal() para obter a implementação do SubHal.
extern "C" __attribute__((visibility("default")))
ISensorsSubHal* sensorsHalGetSubHal(uint32_t* version) {
    *version = SUB_HAL_2_0_VERSION;   // 0x02000000
    return new android::hardware::sensors::V2_0::implementation::LoraSubHal();
}
