#include "lora_service.h"

namespace aidl::devtitans::lora {

    ndk::ScopedAStatus LoraService::connect(int32_t* _aidl_return) {
        *_aidl_return = this->lora.connect();
        LOG(INFO) << "connect(): " << *_aidl_return;
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus LoraService::send(const std::string& in_payload, bool* _aidl_return) {
        *_aidl_return = this->lora.send(in_payload);
        LOG(INFO) << "send(" << in_payload << "): " << (*_aidl_return ? "true" : "false");
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus LoraService::ping(bool* _aidl_return) {
        *_aidl_return = this->lora.ping();
        LOG(INFO) << "ping(): " << (*_aidl_return ? "true" : "false");
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus LoraService::getAux(int32_t* _aidl_return) {
        *_aidl_return = this->lora.getAux();
        LOG(INFO) << "getAux(): " << *_aidl_return;
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus LoraService::getRxCount(int64_t* _aidl_return) {
        *_aidl_return = this->lora.getRxCount();
        LOG(INFO) << "getRxCount(): " << *_aidl_return;
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus LoraService::getLastRx(std::string* _aidl_return) {
        *_aidl_return = this->lora.getLastRx();
        LOG(INFO) << "getLastRx(): " << *_aidl_return;
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus LoraService::getKey(std::string* _aidl_return) {
        *_aidl_return = this->lora.getKey();
        LOG(INFO) << "getKey(): " << *_aidl_return;
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus LoraService::setKey(const std::string& in_key, bool* _aidl_return) {
        *_aidl_return = this->lora.setKey(in_key);
        LOG(INFO) << "setKey(" << in_key << "): " << (*_aidl_return ? "true" : "false");
        return ndk::ScopedAStatus::ok();
    }

    LoraService::~LoraService() {
        running = false;
        if (listener.joinable())
            listener.join();
    }

    ndk::ScopedAStatus LoraService::registerCallback(const std::shared_ptr<ILoraCallback>& in_callback) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (in_callback != nullptr)
                callbacks.push_back(in_callback);
        }

        if (!running.exchange(true))
            listener = std::thread(&LoraService::listenLoop, this);

        LOG(INFO) << "registerCallback()";
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus LoraService::unregisterCallback(const std::shared_ptr<ILoraCallback>& in_callback) {
        std::lock_guard<std::mutex> lock(mtx);
        callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), in_callback), callbacks.end());
        LOG(INFO) << "unregisterCallback()";
        return ndk::ScopedAStatus::ok();
    }

    void LoraService::listenLoop() {
        lastCount = this->lora.getRxCount();

        while (running.load()) {
            long count = this->lora.getRxCount();
            if (count != lastCount) {
                lastCount = count;
                std::string message = this->lora.getLastRx();

                std::vector<std::shared_ptr<ILoraCallback>> current;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    current = callbacks;
                }

                for (const auto& cb : current) {
                    if (cb != nullptr)
                        cb->onMessageReceived(message);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

}
