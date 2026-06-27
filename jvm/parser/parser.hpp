#ifndef PARSER_HPP
#define PARSER_HPP

#include "estrutura_dados.hpp"
#include <string>
#include "constants.hpp"
#include "../leitor/leitor.hpp"

/**
 * @brief Motor principal de decodificação (parsing) do arquivo Java.
 * 
 * Consome os bytes brutos lidos pela classe Leitor e os distribui rigorosamente de 
 * acordo com a Especificação Oficial da JVM (Java Virtual Machine Specification),
 * preenchendo a estrutura 'class_info' (árvore de sintaxe) na memória do C++.
 */
class Parser
{
public:
    explicit Parser(const std::string &filename);
    ~Parser();
    class_info getClassInfo() const { return classInfo; }

    /**
     * @brief Executa o pipeline completo de extração dos dados sequenciais do bytecode.
     * @return class_info contendo toda a árvore da classe pronta para ser lida e exibida.
     */
    class_info parse();

private:
    class_info classInfo;
    Leitor leitor;

    // Métodos de parsing individuais (A ordem das chamadas reflete a estrutura física do arquivo .class)
    void parseMagic();
    void parseMinorVersion();
    void parseMajorVersion();
    void parseConstantPoolCount();
    void parseConstantPool();
    void parseAccessFlags();
    void parseThisClass();
    void parseSuperClass();
    void parseInterfacesCount();
    void parseInterfaces();
    void parseFieldsCount();
    void parseFields();
    void parseMethodsCount();
    void parseMethods();
    void parseAttributesCount();
    void parseAttributes();

    std::string getUtf8Constant(u2 index);
};

#endif // PARSER_HPP
