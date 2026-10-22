#pragma once

#include <android-base/logging.h>
#include <android/binder_process.h>
#include <android/binder_manager.h>

#include <aidl/devtitans/lora/BnLora.h>
#include <aidl/devtitans/lora/ILoraCallback.h>
#include "loracomm_lib.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace devtitans::loracomm;

namespace aidl::devtitans::lora {

    class LoraService : public BnLora {
        public:
            ndk::ScopedAStatus connect(int32_t* _aidl_return) override;
            ndk::ScopedAStatus send(const std::string& in_payload, bool* _aidl_return) override;
            ndk::ScopedAStatus ping(bool* _aidl_return) override;
            ndk::ScopedAStatus getAux(int32_t* _aidl_return) override;
            ndk::ScopedAStatus getRxCount(int64_t* _aidl_return) override;
            ndk::ScopedAStatus getLastRx(std::string* _aidl_return) override;
            ndk::ScopedAStatus getKey(std::string* _aidl_return) override;
            ndk::ScopedAStatus setKey(const std::string& in_key, bool* _aidl_return) override;
            ndk::ScopedAStatus registerCallback(const std::shared_ptr<ILoraCallback>& in_callback) override;
            ndk::ScopedAStatus unregisterCallback(const std::shared_ptr<ILoraCallback>& in_callback) override;
            ~LoraService();

        private:
            Loracomm lora;                                 // Biblioteca

            std::mutex mtx;
            std::vector<std::shared_ptr<ILoraCallback>> callbacks;
            std::atomic<bool> running{false};
            std::thread listener;
            long lastCount = -1;

            void listenLoop();
    };

}
