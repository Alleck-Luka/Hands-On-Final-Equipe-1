#pragma once

#include "V2_0/SubHal.h"   // ISensorsSubHal, IHalProxyCallback, SUB_HAL_2_0_VERSION

#include <atomic>
#include <thread>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

// O pacote 2.0 não re-exporta estes tipos (eles só existem em 1.0), então os
// trazemos explicitamente para o escopo. (Event/Result/SensorInfo já vêm
// "using"ados pelo SubHal.h.)
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::SensorType;
using ::android::hardware::sensors::V1_0::SharedMemInfo;

/**
 * SubHal do LoRa para o MultiHAL de sensores (android.hardware.sensors@2.0).
 *
 * Expõe um sensor numérico customizado (type = DEVICE_PRIVATE_BASE + 1) que
 * reporta uma flag 0 -> 1 sempre que o contador rx_count incrementa (ou seja,
 * sempre que uma mensagem LoRa chega). O SubHal é carregado pelo HalProxy via
 * dlopen do .so que exporta sensorsHalGetSubHal().
 */
class LoraSubHal : public ISensorsSubHal {
  public:
    // ---- ISensorsSubHal ----
    const std::string getName() override { return "LoraSubHal"; }
    Return<Result> initialize(const sp<IHalProxyCallback>& cb) override;

    // ---- ISensors (V2_0, herdados) ----
    Return<void> getSensorsList(ISensors::getSensorsList_cb _hidl_cb) override;
    Return<Result> setOperationMode(OperationMode) override { return Result::OK; }
    Return<Result> activate(int32_t sensorHandle, bool enabled) override;
    Return<Result> batch(int32_t, int64_t, int64_t) override { return Result::OK; }
    Return<Result> flush(int32_t) override { return Result::OK; }
    Return<Result> injectSensorData(const Event&) override { return Result::OK; }
    Return<void> registerDirectChannel(const SharedMemInfo&,
                                       ISensors::registerDirectChannel_cb _hidl_cb) override;
    Return<Result> unregisterDirectChannel(int32_t) override { return Result::OK; }
    Return<void> configDirectReport(int32_t, int32_t, RateLevel,
                                    ISensors::configDirectReport_cb _hidl_cb) override;
    Return<void> debug(const hidl_handle&, const hidl_vec<hidl_string>&) override {
        return Void();
    }

  private:
    // Thread interna que polla o rx_count e gera os eventos (o push real é feito
    // pelo IHalProxyCallback::postEvents, que entrega ao framework).
    void pollLoop();

    // Envia um evento com o valor informado no campo values[0] do sensor.
    void postEvent(float value);

    sp<IHalProxyCallback> mCallback;
    std::thread mThread;
    std::atomic<bool> mActive{false};
    long mLastCount = -1;
    bool mPulseActive = false;   // true enquanto a flag está em "1"
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
