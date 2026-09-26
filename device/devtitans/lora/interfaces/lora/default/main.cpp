#include "lora_service.h"

using namespace aidl::devtitans::lora;
using namespace std;
using namespace ndk;

int main() {
    LOG(INFO) << "Iniciando Lora AIDL Service ...";
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    shared_ptr<LoraService> lora_service = SharedRefBase::make<LoraService>();
    const string instance = string() + ILora::descriptor + "/default";

    binder_status_t status = AServiceManager_addService(lora_service->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    LOG(INFO) << "Lora AIDL Service iniciado com nome: " << instance;
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
