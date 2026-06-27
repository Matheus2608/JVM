// testa se o parser consegue ler corretamente as informações do arquivo Hello.class

#include <iostream>
#include <cassert>
#include "parser.hpp"

int main() {
    Parser parser("exemplos/Hello.class");
    class_info info = parser.parse();

    assert(info.magic_number == 0xCAFEBABE);
    assert(info.minor_version == 0);
    assert(info.major_version == 52);
    assert(info.constant_pool_count == 55);
    assert(info.fields_count == 2);
    assert(info.methods_count == 3);

    std::cout << "Todos os testes passaram." << std::endl;
    return 0;
}
