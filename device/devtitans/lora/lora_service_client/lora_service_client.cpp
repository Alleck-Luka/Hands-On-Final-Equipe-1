#include <android/binder_manager.h>
#include <aidl/devtitans/lora/ILora.h>
#include <iostream>                             // std::cout e std::endl (end-line)
#include <string>                               // std::string

using namespace aidl::devtitans::lora;          // ILora
using namespace std;                            // std::shared_ptr e std::string
using namespace ndk;                            // ndk::SpAIBinder e ndk::ScopedAStatus

// Nome do serviço binder criado pelo HAL (package.nome/instância)
const char *SERVICE_NAME = "devtitans.lora.ILora/default";

// Imprime a sintaxe de uso do cliente
void showUsage(const char *program) {
    cout << "Cliente do serviço Lora (binder)!" << endl;
    cout << "Sintaxe: " << program << " (comando)" << endl;
    cout << "    Comandos: connect, send (payload), ping, get-aux, get-rx-count, get-last-rx, get-key, set-key (key)" << endl;
    cout << "    Obs.: use aspas para mensagens com espaços, ex.: " << program << " send \"ola mundo\"" << endl;
}

int main(int argc, char **argv) {
    // Conecta no serviço binder do HAL do Lora
    shared_ptr<ILora> service;
    service = ILora::fromBinder(SpAIBinder(AServiceManager_getService(SERVICE_NAME)));

    if (!service) {
        cout << "Erro acessando o serviço!" << endl;
        return 1;
    }

    if (argc < 2) {                             // Sem comando: mostra a sintaxe
        showUsage(argv[0]);
        return 1;
    }

    if (argc > 3) {                             // Nenhum comando usa mais de 1 parâmetro
        cout << "Erro: número de argumentos inválido." << endl;
        showUsage(argv[0]);
        return 1;
    }

    string command = argv[1];

    // 1. connect(): retorna 1 se o dispositivo está conectado (driver carregado)
    if (command == "connect") {
        int32_t _aidl_return = 0;
        ScopedAStatus status = service->connect(&_aidl_return);
        if (!status.isOk()) {
            cout << "Erro: " << status.getDescription() << endl;
            return 1;
        }
        cout << (_aidl_return == 1 ? "Dispositivo conectado!" : "Dispositivo não encontrado!") << endl;
    }

    // 2. send(payload): envia uma mensagem via rádio LoRa
    else if (command == "send") {
        if (argc < 3) {
            cout << "Erro: o comando send exige um payload." << endl;
            showUsage(argv[0]);
            return 1;
        }

        string payload = argv[2];
        bool _aidl_return = false;
        ScopedAStatus status = service->send(payload, &_aidl_return);
        if (!status.isOk()) {
            cout << "Erro: " << status.getDescription() << endl;
            return 1;
        }
        cout << (_aidl_return ? "Mensagem enviada: " : "Erro ao enviar mensagem: ") << payload << endl;
    }

    // 3. ping(): envia um PING pelo rádio
    else if (command == "ping") {
        bool _aidl_return = false;
        ScopedAStatus status = service->ping(&_aidl_return);
        if (!status.isOk()) {
            cout << "Erro: " << status.getDescription() << endl;
            return 1;
        }
        cout << (_aidl_return ? "Ping enviado!" : "Erro ao enviar ping") << endl;
    }

    // 4. getAux(): 1 = módulo em modo RX (pronto para receber), 0 = TX
    else if (command == "get-aux") {
        int32_t _aidl_return = 0;
        ScopedAStatus status = service->getAux(&_aidl_return);
        if (!status.isOk()) {
            cout << "Erro: " << status.getDescription() << endl;
            return 1;
        }
        cout << (_aidl_return == 1 ? "Dispositivo em modo de RX!" : "Dispositivo em TX!") << endl;
    }

    // 5. getRxCount(): quantidade de mensagens recebidas
    else if (command == "get-rx-count") {
        int64_t _aidl_return = 0;
        ScopedAStatus status = service->getRxCount(&_aidl_return);
        if (!status.isOk()) {
            cout << "Erro: " << status.getDescription() << endl;
            return 1;
        }
        cout << "Quantidade de mensagens recebidas: " << _aidl_return << endl;
    }

    // 6. getLastRx(): última mensagem recebida
    else if (command == "get-last-rx") {
        string _aidl_return;
        ScopedAStatus status = service->getLastRx(&_aidl_return);
        if (!status.isOk()) {
            cout << "Erro: " << status.getDescription() << endl;
            return 1;
        }
        cout << "Última mensagem: " << _aidl_return << endl;
    }

    // 7. getKey(): chave/senha atual configurada no dispositivo
    else if (command == "get-key") {
        string _aidl_return;
        ScopedAStatus status = service->getKey(&_aidl_return);
        if (!status.isOk()) {
            cout << "Erro: " << status.getDescription() << endl;
            return 1;
        }
        cout << "Key atual do dispositivo: " << _aidl_return << endl;
    }

    // 8. setKey(key): troca a chave/senha do dispositivo
    else if (command == "set-key") {
        if (argc < 3) {
            cout << "Erro: o comando set-key exige uma chave." << endl;
            showUsage(argv[0]);
            return 1;
        }

        string key = argv[2];
        bool _aidl_return = false;
        ScopedAStatus status = service->setKey(key, &_aidl_return);
        if (!status.isOk()) {
            cout << "Erro: " << status.getDescription() << endl;
            return 1;
        }
        cout << (_aidl_return ? "Key atualizada: " : "Erro ao setar key: ") << key << endl;
    }

    else {
        cout << "Comando inválido." << endl;
        showUsage(argv[0]);
        return 1;
    }

    return 0;
}
