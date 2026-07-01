#include <iostream>
#include <string>

#include "class_loader.hpp"
#include "runtime.hpp"
#include "heap.hpp"
#include "interpreter.hpp"
#include "cp_utils.hpp"

/**
 * @brief Ponto de entrada do motor de execução (Execution Engine) da JVM.
 *
 * Este executável recebe o caminho para um arquivo .class, inicializa as áreas
 * de dados de tempo de execução (ClassLoader, Heap, FrameStack), localiza o
 * método `main` e inicia o interpretador de bytecodes.
 */

namespace {

/**
 * @brief Extrai o classpath e o nome da classe a partir de um caminho de arquivo.
 *
 * Transforma um argumento como "exemplos/fatorial.class" em um classpath "exemplos"
 * e um nome de classe "fatorial", permitindo que o ClassLoader localize o arquivo.
 */
void splitPath(const std::string& arg, std::string& classpath, std::string& class_name) {
    std::string path = arg;

    const std::string ext = ".class";
    if (path.size() >= ext.size() &&
        path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
        path = path.substr(0, path.size() - ext.size());

    std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        classpath  = ".";
        class_name = path;
    } else {
        classpath  = path.substr(0, slash);
        class_name = path.substr(slash + 1);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo.class>" << std::endl;
        return 1;
    }

    try {
        std::string classpath, class_name;
        splitPath(argv[1], classpath, class_name);

        // Inicializa as áreas de dados de tempo de execução e o motor de execução.
        ClassLoader loader({classpath, "."});
        FrameStack  frame_stack;
        Heap        heap;
        Interpreter interpreter(loader, frame_stack, heap);

        // Carrega a classe principal e localiza seu método `main`.
        const class_info& cls = loader.load(class_name);
        const method_info* main_method =
            findMethod(cls, "main", "([Ljava/lang/String;)V");
        if (!main_method)
            throw std::runtime_error("Classe nao possui metodo 'main' (static void main(String[]))");

        // Inicia a execução a partir do método main.
        interpreter.execute(cls, *main_method);
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
