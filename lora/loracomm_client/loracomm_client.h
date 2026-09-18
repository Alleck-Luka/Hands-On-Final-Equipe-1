#pragma once // Inclui esse cabeçalho apenas uma vez

#include <iostream> // std::cout (char-out) e std::endl (end-line)
#include <string.h> // Função strcmp
#include <stdlib.h> // Função atoi

#include "loracomm_lib.h" // Classe Loracomm

namespace devtitans::loracomm
{ // Pacote que a classe abaixo pertence

  class LoracommClient
  { // Classe

  public:
    void start(int argc, char **argv);
  };

} // namespace