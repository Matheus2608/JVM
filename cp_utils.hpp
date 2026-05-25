#ifndef CP_UTILS_HPP
#define CP_UTILS_HPP

#include <string>
#include <stdexcept>
#include <cstring>
#include "estrutura_dados.hpp"
#include "constants.hpp"
#include "bytecode_utils.hpp"

inline std::string utf8FromConstantPool(const class_info &info, u2 index)
{
    if (index == 0 || index >= info.constant_pool.size())
        throw std::runtime_error("Exibidor: indice invalido na constant pool");

    const cp_info &entry = info.constant_pool[index];
    if (entry.tag != CONSTANT_Utf8)
        throw std::runtime_error("Exibidor: entrada da constant pool nao e Utf8");

    return entry.utf8_str;
}

inline std::string classNameFromConstantPool(const class_info &info, u2 index)
{
    if (index == 0 || index >= info.constant_pool.size())
        throw std::runtime_error("Exibidor: indice invalido de classe na constant pool");

    const cp_info &entry = info.constant_pool[index];
    if (entry.tag != CONSTANT_Class)
        throw std::runtime_error("Exibidor: entrada da constant pool nao e Class");

    return utf8FromConstantPool(info, entry.container.Class.name_index);
}

inline std::string nameAndTypeStr(const class_info &info, u2 index)
{
    if (index == 0 || index >= info.constant_pool.size()) return "";
    const cp_info &entry = info.constant_pool[index];
    if (entry.tag != CONSTANT_NameAndType) return "";
    return utf8FromConstantPool(info, entry.container.NameAndType.name_index) + ":" +
           utf8FromConstantPool(info, entry.container.NameAndType.descriptor_index);
}

inline std::string cpEntryComment(const class_info &info, u2 index)
{
    if (index == 0 || index >= info.constant_pool.size()) return "";
    const cp_info &entry = info.constant_pool[index];
    switch (entry.tag) {
    case CONSTANT_Class:
        return utf8FromConstantPool(info, entry.container.Class.name_index);
        
    case CONSTANT_Module:
    case CONSTANT_Package:
        return utf8FromConstantPool(info, entry.container.Class.name_index);
    
    case CONSTANT_Fieldref:
    case CONSTANT_Methodref:
    case CONSTANT_InterfaceMethodref:
        return classNameFromConstantPool(info, entry.container.Fieldref.class_index) + "." +
               nameAndTypeStr(info, entry.container.Fieldref.name_and_type_index);
    case CONSTANT_String:
        return utf8FromConstantPool(info, entry.container.String.string_index);
    case CONSTANT_NameAndType:
        return nameAndTypeStr(info, index);
        
    case CONSTANT_MethodType:
        return utf8FromConstantPool(info, entry.container.MethodType.descriptor_index);
        
    case CONSTANT_MethodHandle: {
        u1 kind = entry.container.MethodHandle.reference_kind;
        u2 ref_idx = entry.container.MethodHandle.reference_index;
        std::string kind_str;
        switch(kind) {
            case 1: kind_str = "REF_getField"; break;
            case 2: kind_str = "REF_getStatic"; break;
            case 3: kind_str = "REF_putField"; break;
            case 4: kind_str = "REF_putStatic"; break;
            case 5: kind_str = "REF_invokeVirtual"; break;
            case 6: kind_str = "REF_invokeStatic"; break;
            case 7: kind_str = "REF_invokeSpecial"; break;
            case 8: kind_str = "REF_newInvokeSpecial"; break;
            case 9: kind_str = "REF_invokeInterface"; break;
            default: kind_str = "REF_Unknown";
        }
        return kind_str + " -> " + cpEntryComment(info, ref_idx);
    }
    
    case CONSTANT_InvokeDynamic: {
        u2 bsm_idx = entry.container.InvokeDynamic.bootstrap_method_attr_index;
        u2 nat_idx = entry.container.InvokeDynamic.name_and_type_index;
        return "BootstrapMethod #" + std::to_string(bsm_idx) + " : " + nameAndTypeStr(info, nat_idx);
    }

    case CONSTANT_Integer:
        return std::to_string(static_cast<int32_t>(entry.container.Integer.bytes));
    case CONSTANT_Float: {
        float val;
        u4 bits = entry.container.Float.bytes;
        memcpy(&val, &bits, sizeof(float));
        return std::to_string(val) + "f";
    }
    case CONSTANT_Long: {
        int64_t val = static_cast<int64_t>(
            (static_cast<uint64_t>(entry.container.Long.high_bytes) << 32) |
             entry.container.Long.low_bytes);
        return std::to_string(val) + "l";
    }
    case CONSTANT_Double: {
        uint64_t bits = (static_cast<uint64_t>(entry.container.Double.high_bytes) << 32) |
                         entry.container.Double.low_bytes;
        double val;
        memcpy(&val, &bits, sizeof(double));
        return std::to_string(val);
    }
    default:
        return "";
    }
}

#endif // CP_UTILS_HPP