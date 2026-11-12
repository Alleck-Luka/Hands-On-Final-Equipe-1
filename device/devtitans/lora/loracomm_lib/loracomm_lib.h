#pragma once

#include <fstream>                      // Classe ifstream
#include <android-base/properties.h>    // Função GetBoolProperty
#include <sys/stat.h>
#include <string>

using namespace std; // Permite usar string diretamente ao invés de std::string

namespace devtitans::loracomm
{

  class Loracomm
  {
  public:
    /**
     * Verifica se o diretório /sys/kernel/loracomm existe. Se existir
     * significa que o dispositivo está conectado e o driver foi carregado.
     *
     * Retorna:
     *      0: dispositivo não encontrado
     *      1: sucesso
     */
    int connect();

    // retorna true se conseguir
    bool send(string payload);

    // retorna true se conseguir
    bool ping();

    // Coloca o ESP32 e o módulo LoRa em Deep Sleep
    bool sleep();

    // Acorda o dispositivo puxando a linha RX
    bool wakeup();

    // Verifica se o dispositivo está responsivo
    bool isAwake();

    // retorna true se o buffer de envio estiver livre (ou seja, se o módulo estiver em modo RX)
    bool getAux();

    // quantidade de mensagens recebidas
    long getRxCount();

    // última mensagem recebida
    string getLastRx();

    // recebe a key atual do dispositivo
    string getKey();

    // envia uma nova key para o dispositivo
    bool setKey(string key);

  private:
    string readFileValue(string file);
    bool writeFileValue(string file, string value);
  };

} // namespace

