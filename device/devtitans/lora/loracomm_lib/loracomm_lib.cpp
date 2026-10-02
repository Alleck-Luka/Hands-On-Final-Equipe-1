#include "loracomm_lib.h"
#include <cstdlib> // strtol / strtoll (conversões sem exceções)

using namespace std; // Permite usar string, ifstream diretamente ao invés de std::string

namespace devtitans::loracomm
{ // Entra no pacote devtitans::loracomm

  // Conversões seguras: sem o dispositivo conectado o readFileValue() retorna "".
  // Usamos strtol/strtoll (e não stoi/stol) porque o Android compila código vendor
  // com -fno-exceptions (try/catch não é permitido). strtol retorna 0 para valores
  // vazios ou não numéricos, sem nunca lançar exceção.
  static int toInt(const string &value)
  {
    return value.empty() ? 0 : static_cast<int>(strtol(value.c_str(), nullptr, 10));
  }

  static long toLong(const string &value)
  {
    return value.empty() ? 0 : strtol(value.c_str(), nullptr, 10);
  }

  int Loracomm::connect()
  {
    char dirPath[] = "/sys/kernel/loracomm";
    struct stat dirStat;
    if (stat(dirPath, &dirStat) == 0)
      if (S_ISDIR(dirStat.st_mode))
        return 1; // Se o diretório existir, retorna 1
    
    return 0;
  }

  string Loracomm::readFileValue(string file)
  {
    int connected = this->connect();

    if (connected == 1)
    { // Conectado. Vamos solicitar o valor ao dispositivo
      string value;
      string filename = string("/sys/kernel/loracomm/") + file;
      ifstream file(filename); // Abre o arquivo do módulo do kernel

      if (file.is_open())
      {                       // Verifica se o arquivo foi aberto com sucesso
        getline(file, value); // Lê uma linha do arquivo
        file.close();
        return value;
      }
    }

    // Se chegou aqui, não foi possível conectar ou se comunicar com o dispositivo
    return "";
  }

  bool Loracomm::writeFileValue(string file, string value)
  {
    int connected = this->connect();

    if (connected == 1)
    { // Conectado. Vamos solicitar o valor ao dispositivo
      string filename = string("/sys/kernel/loracomm/") + file;
      ofstream file(filename, ios::trunc); // Abre o arquivo limpando o seu conteúdo

      if (file.is_open())
      {                // Verifica se o arquivo foi aberto com sucesso
        file << value; // Escreve no arquivo
        file.close();
        return true;
      }
    }

    // Se chegou aqui, não foi possível conectar ou se comunicar com o dispositivo
    return false;
  }

  bool Loracomm::send(string payload)
  {
    return this->writeFileValue("send", payload);
  }

  bool Loracomm::ping() {
      string val = this->readFileValue("ping");
      return toInt(val) != 0;
  }

  bool Loracomm::getAux() {
      string val = this->readFileValue("aux");
      return toInt(val) != 0;
  }

  long Loracomm::getRxCount() {
      string val = this->readFileValue("rx_count");
      return toLong(val);
  }

  string Loracomm::getLastRx()
  {
    return this->readFileValue("last_rx");
  }

  string Loracomm::getKey()
  {
    return this->readFileValue("key");
  }

  bool Loracomm::setKey(string key)
  {
    return this->writeFileValue("key", key);
  }

}
