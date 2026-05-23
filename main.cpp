#include <iostream>
#include <string>
#include "exibidor.hpp"

using namespace std;

int main(int argc, char* argv[]) {

    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <arquivo.class>" << endl;
        return 1;
    }

    try {
        Exibidor exibidor(argv[1]);
        exibidor.display();
    } catch (const std::exception &e) {
        cerr << "Erro: " << e.what() << endl;
        return 1;
    }

    return 0;
}