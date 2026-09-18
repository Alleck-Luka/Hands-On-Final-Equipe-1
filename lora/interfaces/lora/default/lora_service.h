#pragma once

#include <android-base/logging.h>
#include <android/binder_process.h>
#include <android/binder_manager.h>

#include <aidl/devtitans/lora/BnLora.h>
#include "lora_lib.h"

using namespace devtitans::lora;

namespace aidl::devtitans::lora {

class LoraService : public BnLora {
    public:
        ndk::ScopedAStatus connect(int32_t* _aidl_return) override;
        ndk::ScopedAStatus send(const std::string& in_payload, bool* _aidl_return) override;
        ndk::ScopedAStatus ping(bool* _aidl_return) override;
        ndk::ScopedAStatus getAux(int32_t* _aidl_return) override;
        ndk::ScopedAStatus getRxCount(int64_t* _aidl_return) override;
        ndk::ScopedAStatus getLastRx(std::string* _aidl_return) override;
    private:
        Lora lora;                                 // Biblioteca
};

}