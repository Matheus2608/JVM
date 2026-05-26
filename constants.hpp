/**
 * @brief Definições de Tags da Constant Pool e Mapeamento de Modificadores (Access Flags).
 * 
 * Contém as constantes literais definidas pela especificação da JVM para 
 * identificar o tipo de cada entrada na Constant Pool, além das máscaras 
 * de bits (bitwise flags) usadas para descobrir se uma classe, campo ou método 
 * é public, private, static, abstrato, etc.
 */
#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>
#include <cstdint>

#include "estrutura_dados.hpp"

#define CONSTANT_Class              7
#define CONSTANT_Fieldref           9
#define CONSTANT_Methodref          10
#define CONSTANT_InterfaceMethodref 11
#define CONSTANT_String             8
#define CONSTANT_Integer            3
#define CONSTANT_Float              4
#define CONSTANT_Long               5
#define CONSTANT_Double             6
#define CONSTANT_NameAndType        12
#define CONSTANT_Utf8               1
#define CONSTANT_MethodHandle       15
#define CONSTANT_MethodType         16
#define CONSTANT_InvokeDynamic      18
#define CONSTANT_Module             19
#define CONSTANT_Package 20


// Access Flags (podem ser combinadas com bitwise OR)
enum class AccessFlags : u2 {
    ACC_PUBLIC      = 0x0001,  // Declarada pública
    ACC_PRIVATE     = 0x0002,  // Declarada privada
    ACC_PROTECTED   = 0x0004,  // Declarada protegida
    ACC_STATIC      = 0x0008,  // Static
    ACC_FINAL       = 0x0010,  // Declarada final
    ACC_SUPER       = 0x0020,  // Chama métodos de superclasse (classe)
    ACC_SYNCHRONIZED= 0x0020,  // Synchronized (método)
    ACC_VOLATILE    = 0x0040,  // Volatile (campo)
    ACC_BRIDGE      = 0x0040,  // Bridge method
    ACC_TRANSIENT   = 0x0080,  // Transient (campo)
    ACC_VARARGS     = 0x0080,  // Varargs
    ACC_NATIVE      = 0x0100,  // Native
    ACC_INTERFACE   = 0x0200,  // É interface
    ACC_ABSTRACT    = 0x0400,  // Declarada abstrata
    ACC_STRICT      = 0x0800,  // Strictfp
    ACC_SYNTHETIC   = 0x1000,  // Não presente no fonte
    ACC_ANNOTATION  = 0x2000,  // Tipo annotation
    ACC_ENUM        = 0x4000   // Tipo enum
};

inline bool hasAccessFlag(u2 flags, AccessFlags flag) {
    return (flags & static_cast<u2>(flag)) != 0;
}

inline std::string finalizeAccessFlagsString(std::string result) {
    if (!result.empty()) {
        result.pop_back();
    }
    if (result.empty()) {
        return "package-private";
    }
    return result;
}

inline std::string getClassAccessFlagsString(u2 flags) {
    std::string result;
    if (hasAccessFlag(flags, AccessFlags::ACC_PUBLIC))    result += "public ";
    if (hasAccessFlag(flags, AccessFlags::ACC_FINAL))     result += "final ";
    if (hasAccessFlag(flags, AccessFlags::ACC_SUPER))     result += "super ";
    if (hasAccessFlag(flags, AccessFlags::ACC_INTERFACE)) result += "interface ";
    if (hasAccessFlag(flags, AccessFlags::ACC_ABSTRACT))  result += "abstract ";
    if (hasAccessFlag(flags, AccessFlags::ACC_SYNTHETIC)) result += "synthetic ";
    if (hasAccessFlag(flags, AccessFlags::ACC_ANNOTATION))result += "annotation ";
    if (hasAccessFlag(flags, AccessFlags::ACC_ENUM))      result += "enum ";
    return finalizeAccessFlagsString(result);
}

inline std::string getMethodAccessFlagsString(u2 flags) {
    std::string result;
    if (hasAccessFlag(flags, AccessFlags::ACC_PUBLIC))       result += "public ";
    if (hasAccessFlag(flags, AccessFlags::ACC_PRIVATE))      result += "private ";
    if (hasAccessFlag(flags, AccessFlags::ACC_PROTECTED))    result += "protected ";
    if (hasAccessFlag(flags, AccessFlags::ACC_STATIC))       result += "static ";
    if (hasAccessFlag(flags, AccessFlags::ACC_FINAL))        result += "final ";
    if (hasAccessFlag(flags, AccessFlags::ACC_SYNCHRONIZED)) result += "synchronized ";
    if (hasAccessFlag(flags, AccessFlags::ACC_BRIDGE))       result += "bridge ";
    if (hasAccessFlag(flags, AccessFlags::ACC_VARARGS))      result += "varargs ";
    if (hasAccessFlag(flags, AccessFlags::ACC_NATIVE))       result += "native ";
    if (hasAccessFlag(flags, AccessFlags::ACC_ABSTRACT))     result += "abstract ";
    if (hasAccessFlag(flags, AccessFlags::ACC_STRICT))       result += "strictfp ";
    if (hasAccessFlag(flags, AccessFlags::ACC_SYNTHETIC))    result += "synthetic ";
    return finalizeAccessFlagsString(result);
}

inline std::string getFieldAccessFlagsString(u2 flags) {
    std::string result;
    if (hasAccessFlag(flags, AccessFlags::ACC_PUBLIC))    result += "public ";
    if (hasAccessFlag(flags, AccessFlags::ACC_PRIVATE))   result += "private ";
    if (hasAccessFlag(flags, AccessFlags::ACC_PROTECTED)) result += "protected ";
    if (hasAccessFlag(flags, AccessFlags::ACC_STATIC))    result += "static ";
    if (hasAccessFlag(flags, AccessFlags::ACC_FINAL))     result += "final ";
    if (hasAccessFlag(flags, AccessFlags::ACC_VOLATILE))  result += "volatile ";
    if (hasAccessFlag(flags, AccessFlags::ACC_TRANSIENT)) result += "transient ";
    if (hasAccessFlag(flags, AccessFlags::ACC_SYNTHETIC)) result += "synthetic ";
    if (hasAccessFlag(flags, AccessFlags::ACC_ENUM))      result += "enum ";
    return finalizeAccessFlagsString(result);
}

#endif // CONSTANTS_HPP
