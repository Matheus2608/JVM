#ifndef PARSER_HPP
#define PARSER_HPP
#define CONSTANT_Module 19
#define CONSTANT_Package 20

#include "estrutura_dados.hpp"
#include <string>
#include "constants.hpp"
#include "leitor.hpp"

class Parser
{
public:
    explicit Parser(const std::string &filename);
    ~Parser();
    class_info getClassInfo() const { return classInfo; }

    class_info parse();

private:
    class_info classInfo;
    Leitor leitor;

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
