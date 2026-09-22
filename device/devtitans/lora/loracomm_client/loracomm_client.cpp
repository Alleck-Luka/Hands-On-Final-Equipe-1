#include "loracomm_client.h"

using namespace std; // Permite usar o cout e endl diretamente ao invés de std::cout

namespace devtitans::loracomm
{ // Entra no pacote devtitans::hello

  void LoracommClient::start(int argc, char **argv)
  {
    cout << "Cliente Loracomm!" << endl;

    if (argc < 2)
    {
      cout << "Sintaxe: " << argv[0] << "  " << endl;
      cout << "    Comandos: send (payload) #use aspas para mensagens com espaços, ping, get-aux, get-rx-count, get-last-rx, get-key, set-key (key)" << endl;
      exit(1);
    }

    if (argc > 3)
    {
      cout << "Erro: número de argumentos inválido." << endl;
      exit(1);
    }

    Loracomm loracomm; // Classe da biblioteca Loracomm

    if (!strcmp(argv[1], "send"))
    {
      string msg = argv[2];
      if (loracomm.send(msg))
      {
        cout << "Mensagem enviada: " << msg << endl;
      }
      else
      {
        cout << "Erro ao enviar mensagem: " << msg << endl;
      }
    }
    else if (!strcmp(argv[1], "set-key"))
    {
      string key = argv[2];
      if (loracomm.setKey(key))
      {
        cout << "Key atualizada: " << key << endl;
      }
      else
      {
        cout << "Erro ao setar key: " << key << endl;
      }
    }
    else if (argc > 2)
    {
      cout << "Numero de argumentos inválido!";
    }
    else if (!strcmp(argv[1], "ping"))
    {
      cout << (loracomm.ping() ? "Ping enviado!" : "Erro ao enviar ping") << endl;
    }
    else if (!strcmp(argv[1], "get-aux"))
    {
      cout << (loracomm.getAux() ? "Dispositivo em modo de RX!" : "Dispositivo em TX!") << endl;
    }
    else if (!strcmp(argv[1], "get-key"))
    {
      cout << "Key atual do dispositivo: " << loracomm.getKey() << endl;
    }
    else if (!strcmp(argv[1], "get-last-rx"))
    {
      cout << "Última mensagem: " << loracomm.getLastRx() << endl;
    }
    else if (!strcmp(argv[1], "get-rx-count"))
    {
      cout << "Quantidade de mensagens recebidas: " << loracomm.getRxCount() << endl;
    }
    else
    {
      cout << "Comando inválido." << endl;
      exit(1);
    }
  }

} // namespace

// MAIN

using namespace devtitans::loracomm; // Permite usar LoracommClient diretamente ao invés de devtitans::loracomm::LoracommClient

int main(int argc, char **argv)
{
  LoracommClient client;    // Variável client, da classe LoracommClient, do pacote devtitans::loracomm
  client.start(argc, argv); // Executa o método start
  return 0;
}
