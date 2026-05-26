/**
 * @brief Estruturas de dados (AST) que espelham o formato físico do arquivo .class.
 * 
 * Este arquivo funciona como o modelo de domínio do projeto. As structs aqui definidas
 * mapeiam byte-a-byte a especificação oficial da Oracle para a JVM, permitindo carregar
 * toda a classe em memória de forma tipada e estruturada.
 */
#ifndef ESTRUTURA_DADOS_HPP
#define ESTRUTURA_DADOS_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;

/**
 * @brief Representa uma entrada genérica na Pool de Constantes.
 * Usa uma union (container) para economizar memória e mapear múltiplos tipos JVM baseando-se na tag.
 */
struct cp_info {
    u1 tag = 0;
    union {
        struct { u2 name_index; } Class;
        struct { u2 class_index; u2 name_and_type_index; } Fieldref, Methodref, InterfaceMethodref;
        struct { u2 string_index; } String;
        struct { u4 bytes; } Integer, Float;
        struct { u4 high_bytes; u4 low_bytes; } Long, Double;
        struct { u1 reference_kind; u2 reference_index; } MethodHandle;
        struct { u2 descriptor_index; } MethodType;
        struct { u2 bootstrap_method_attr_index; u2 name_and_type_index; } InvokeDynamic;
        struct { u2 name_index; u2 descriptor_index; } NameAndType;
    } container;
    std::string utf8_str;
};

struct code_attribute;

struct attribute_info {
    u2 attribute_name_index = 0;
    u4 attribute_length = 0;
    std::vector<u1> info;
    std::shared_ptr<code_attribute> code_data;
};

struct exception_table_info {
    u2 start_pc;
    u2 end_pc;
    u2 handler_pc;
    u2 catch_type;
};

struct code_attribute {
    u2 attribute_name_index = 0;
    u4 attribute_length = 0;
    u2 max_stack = 0;
    u2 max_locals = 0;
    u4 code_length = 0;
    std::vector<u1> code;
    u2 exception_table_length = 0;
    std::vector<exception_table_info> exception_table;
    u2 attributes_count = 0;
    std::vector<attribute_info> attributes;
};

struct field_info {
    u2 access_flags = 0;
    u2 name_index = 0;
    u2 descriptor_index = 0;
    u2 attributes_count = 0;
    std::vector<attribute_info> attributes;
};

struct method_info {
    u2 access_flags = 0;
    u2 name_index = 0;
    u2 descriptor_index = 0;
    u2 attributes_count = 0;
    std::vector<attribute_info> attributes;
};

/**
 * @brief Nó raiz da estrutura do arquivo .class.
 * Contém toda a hierarquia de dados extraída do binário.
 */
struct class_info {
    u4 magic_number = 0;
    u2 minor_version = 0;
    u2 major_version = 0;
    u2 constant_pool_count = 0;
    std::vector<cp_info> constant_pool;
    u2 access_flags = 0;
    u2 this_class = 0;
    u2 super_class = 0;
    u2 interfaces_count = 0;
    std::vector<u2> interfaces;
    u2 fields_count = 0;
    std::vector<field_info> fields;
    u2 methods_count = 0;
    std::vector<method_info> methods;
    u2 attributes_count = 0;
    std::vector<attribute_info> attributes;
};

#endif // ESTRUTURA_DADOS_HPP
