#ifndef EXIBIDOR_HPP
#define EXIBIDOR_HPP

#include <fstream>
#include <string>
#include <utility>

#include "estrutura_dados.hpp"
#include "constants.hpp"
#include "parser.hpp"

/**
 * @brief Formata e exibe o conteúdo de um arquivo .class de forma legível.
 * 
 * Utiliza as informações da Constant Pool para resolver nomes de classes,
 * métodos e campos, apresentando uma visão estruturada do bytecode.
 */
class Exibidor {
public:
    explicit Exibidor(const std::string& filename);
    ~Exibidor();

    /** @brief Exibe todas as seções do arquivo .class no terminal. */
    void display();

private:
    std::string filename;
    class_info classInfo;

    // Blocos modulares de exibição visual
    void constantPoolDisplay();
    void fieldsDisplay();
    void methodsDisplay();
    void attributesDisplay();
    
    /**
     * @brief Converte um descritor de método da JVM (ex: "(II)V") em uma
     * assinatura Java legível (ex: "void nome(int, int)").
     */
    std::pair<std::string, std::string> getMethodSignature(const method_info &method);
};

#endif // EXIBIDOR_HPP
