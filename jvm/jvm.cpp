#include <iostream>
#include <string>

#include "class_loader.hpp"
#include "runtime.hpp"
#include "heap.hpp"
#include "interpreter.hpp"
#include "cp_utils.hpp"

// =============================================================================
// jvm — ponto de entrada do Execution Engine
//
// Recebe o caminho de um .class, carrega a classe, localiza o método `main`
// e dispara o interpretador. É a contraparte "executora" do Leitor-Exibidor
// (que apenas exibe o conteúdo do .class).
//
// Uso: ./bin/jvm <caminho/Classe.class>
//   ex.: ./bin/jvm exemplos/fatorial.class
// =============================================================================

namespace {

// Separa "exemplos/fatorial.class" em classpath ("exemplos") e nome ("fatorial").
void splitPath(const std::string& arg, std::string& classpath, std::string& class_name) {
    std::string path = arg;

    // Remove o sufixo .class, se presente.
    const std::string ext = ".class";
    if (path.size() >= ext.size() &&
        path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
        path = path.substr(0, path.size() - ext.size());

    // Separa diretório do nome da classe.
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

        // Runtime Data Area + Execution Engine.
        ClassLoader loader({classpath, "."});
        FrameStack  frame_stack;
        Heap        heap;
        Interpreter interpreter(loader, frame_stack, heap);

        // Carrega a classe e localiza o método main.
        const class_info& cls = loader.load(class_name);
        const method_info* main_method =
            findMethod(cls, "main", "([Ljava/lang/String;)V");
        if (!main_method)
            throw std::runtime_error("Classe nao possui metodo 'main' (static void main(String[]))");

        interpreter.execute(cls, *main_method);
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
