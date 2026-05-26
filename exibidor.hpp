#ifndef EXIBIDOR_HPP
#define EXIBIDOR_HPP

#include <fstream>
#include <string>
#include <utility>

#include "estrutura_dados.hpp"
#include "constants.hpp"
#include "parser.hpp"

/**
 * @brief Responsável por formatar e imprimir as informações da classe.
 * 
 * Realiza a ponte entre os metadados brutos armazenados (ex: números hexadecimais) 
 * e a representação legível para humanos, fazendo ligações dinâmicas com a Constant Pool
 * para exibir os nomes reais de classes, métodos e variáveis referenciadas no código.
 */
class Exibidor {
public:
    explicit Exibidor(const std::string& filename);
    ~Exibidor();

    /**
     * @brief Função orquestradora que chama todas as rotinas de exibição em tela no terminal.
     */
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
     * @brief Traduz as strings difíceis da JVM para uma assinatura de método Java tradicional.
     */
    std::pair<std::string, std::string> getMethodSignature(const method_info &method);
};

#endif // EXIBIDOR_HPP
