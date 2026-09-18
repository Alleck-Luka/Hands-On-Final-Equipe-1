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

}