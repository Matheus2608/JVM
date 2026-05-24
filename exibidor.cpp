#include "exibidor.hpp"
#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <cstring>

namespace
{
    std::string utf8FromConstantPool(const class_info &info, u2 index)
    {
        if (index == 0 || index >= info.constant_pool.size())
        {
            throw std::runtime_error("Exibidor: indice invalido na constant pool");
        }

        const cp_info &entry = info.constant_pool[index];
        if (entry.tag != CONSTANT_Utf8)
        {
            throw std::runtime_error("Exibidor: entrada da constant pool nao e Utf8");
        }

        return entry.utf8_str;
    }

    std::string classNameFromConstantPool(const class_info &info, u2 index)
    {
        if (index == 0 || index >= info.constant_pool.size())
        {
            throw std::runtime_error("Exibidor: indice invalido de classe na constant pool");
        }

        const cp_info &entry = info.constant_pool[index];
        if (entry.tag != CONSTANT_Class)
        {
            throw std::runtime_error("Exibidor: entrada da constant pool nao e Class");
        }

        return utf8FromConstantPool(info, entry.container.Class.name_index);
    }

    std::string parseTypeDescriptor(const std::string &descriptor, size_t &position)
    {
        size_t arrayDimensions = 0;
        while (position < descriptor.size() && descriptor[position] == '[')
        {
            ++arrayDimensions;
            ++position;
        }

        if (position >= descriptor.size())
        {
            throw std::runtime_error("Exibidor: descriptor invalido");
        }

        std::string baseType;
        char current = descriptor[position++];
        switch (current)
        {
        case 'B':
            baseType = "byte";
            break;
        case 'C':
            baseType = "char";
            break;
        case 'D':
            baseType = "double";
            break;
        case 'F':
            baseType = "float";
            break;
        case 'I':
            baseType = "int";
            break;
        case 'J':
            baseType = "long";
            break;
        case 'S':
            baseType = "short";
            break;
        case 'Z':
            baseType = "boolean";
            break;
        case 'V':
            baseType = "void";
            break;
        case 'L':
        {
            size_t end = descriptor.find(';', position);
            if (end == std::string::npos)
            {
                throw std::runtime_error("Exibidor: descriptor de referencia invalido");
            }
            baseType = descriptor.substr(position, end - position);
            position = end + 1;
            break;
        }
        default:
            throw std::runtime_error("Exibidor: tipo de descriptor desconhecido");
        }

        for (size_t i = 0; i < arrayDimensions; ++i)
        {
            baseType += "[]";
        }

        return baseType;
    }

    std::string formatMethodParameters(const std::string &descriptor)
    {
        size_t openParen = descriptor.find('(');
        size_t closeParen = descriptor.find(')');
        if (openParen == std::string::npos || closeParen == std::string::npos || closeParen < openParen)
        {
            throw std::runtime_error("Exibidor: assinatura de metodo invalida");
        }

        std::string result;
        size_t position = openParen + 1;
        bool first = true;

        while (position < closeParen)
        {
            std::string type = parseTypeDescriptor(descriptor, position);

            if (!first)
            {
                result += ", ";
            }
            result += type;
            first = false;
        }

        return result;
    }

    std::string formatMethodReturnType(const std::string &descriptor)
    {
        size_t closeParen = descriptor.find(')');
        if (closeParen == std::string::npos || closeParen + 1 >= descriptor.size())
        {
            throw std::runtime_error("Exibidor: tipo de retorno invalido");
        }

        size_t position = closeParen + 1;
        return parseTypeDescriptor(descriptor, position);
    }

    std::string formatFieldType(const std::string &descriptor)
    {
        size_t position = 0;
        std::string type = parseTypeDescriptor(descriptor, position);
        if (position != descriptor.size())
        {
            throw std::runtime_error("Exibidor: descriptor de campo invalido");
        }

        return type;
    }

    std::string nameAndTypeStr(const class_info &info, u2 index)
    {
        if (index == 0 || index >= info.constant_pool.size()) return "";
        const cp_info &entry = info.constant_pool[index];
        if (entry.tag != CONSTANT_NameAndType) return "";
        return utf8FromConstantPool(info, entry.container.NameAndType.name_index) + ":" +
               utf8FromConstantPool(info, entry.container.NameAndType.descriptor_index);
    }

    std::string cpEntryComment(const class_info &info, u2 index)
    {
        if (index == 0 || index >= info.constant_pool.size()) return "";
        const cp_info &entry = info.constant_pool[index];
        switch (entry.tag) {
        case CONSTANT_Class:
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
        case CONSTANT_Integer:
            return std::to_string(static_cast<int32_t>(entry.container.Integer.bytes));
        case CONSTANT_Float: {
            float val; u4 bits = entry.container.Float.bytes;
            memcpy(&val, &bits, sizeof(float));
            return std::to_string(val) + "f";
        }
        case CONSTANT_Long: {
            int64_t val = (int64_t)(((uint64_t)entry.container.Long.high_bytes << 32)
                                    | entry.container.Long.low_bytes);
            return std::to_string(val) + "l";
        }
        case CONSTANT_Double: {
            uint64_t bits = ((uint64_t)entry.container.Double.high_bytes << 32)
                            | entry.container.Double.low_bytes;
            double val; memcpy(&val, &bits, sizeof(double));
            return std::to_string(val);
        }
        default:
            return "";
        }
    }

    int16_t readS2(const std::vector<u1> &c, size_t i)
    {
        return (int16_t)(((uint16_t)c[i] << 8) | c[i + 1]);
    }

    int32_t readS4(const std::vector<u1> &c, size_t i)
    {
        return (int32_t)(((uint32_t)c[i] << 24) | ((uint32_t)c[i+1] << 16)
                        | ((uint32_t)c[i+2] << 8) | c[i+3]);
    }

    uint16_t readU2(const std::vector<u1> &c, size_t i)
    {
        return (uint16_t)(((uint16_t)c[i] << 8) | c[i + 1]);
    }

    void disassembleCode(const std::vector<u1> &code, u4 code_length, const class_info &cls)
    {
        using namespace std;
        size_t pc = 0;
        while (pc < code_length) {
            u1 op = code[pc];
            cout << "      " << setw(4) << pc << ": ";

            switch (op) {
            case 0x00: cout << "nop";          pc++; break;
            case 0x01: cout << "aconst_null";  pc++; break;
            case 0x02: cout << "iconst_m1";    pc++; break;
            case 0x03: cout << "iconst_0";     pc++; break;
            case 0x04: cout << "iconst_1";     pc++; break;
            case 0x05: cout << "iconst_2";     pc++; break;
            case 0x06: cout << "iconst_3";     pc++; break;
            case 0x07: cout << "iconst_4";     pc++; break;
            case 0x08: cout << "iconst_5";     pc++; break;
            case 0x09: cout << "lconst_0";     pc++; break;
            case 0x0A: cout << "lconst_1";     pc++; break;
            case 0x0B: cout << "fconst_0";     pc++; break;
            case 0x0C: cout << "fconst_1";     pc++; break;
            case 0x0D: cout << "fconst_2";     pc++; break;
            case 0x0E: cout << "dconst_0";     pc++; break;
            case 0x0F: cout << "dconst_1";     pc++; break;

            case 0x10: cout << "bipush " << (int)(int8_t)code[pc+1]; pc += 2; break;
            case 0x11: cout << "sipush " << (int)readS2(code, pc+1); pc += 3; break;

            case 0x12: { u1 idx = code[pc+1];
                cout << "ldc #" << (int)idx << "   // " << cpEntryComment(cls, idx);
                pc += 2; break; }
            case 0x13: { u2 idx = readU2(code, pc+1);
                cout << "ldc_w #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0x14: { u2 idx = readU2(code, pc+1);
                cout << "ldc2_w #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }

            case 0x15: cout << "iload "  << (int)code[pc+1]; pc += 2; break;
            case 0x16: cout << "lload "  << (int)code[pc+1]; pc += 2; break;
            case 0x17: cout << "fload "  << (int)code[pc+1]; pc += 2; break;
            case 0x18: cout << "dload "  << (int)code[pc+1]; pc += 2; break;
            case 0x19: cout << "aload "  << (int)code[pc+1]; pc += 2; break;

            case 0x1A: cout << "iload_0"; pc++; break;
            case 0x1B: cout << "iload_1"; pc++; break;
            case 0x1C: cout << "iload_2"; pc++; break;
            case 0x1D: cout << "iload_3"; pc++; break;
            case 0x1E: cout << "lload_0"; pc++; break;
            case 0x1F: cout << "lload_1"; pc++; break;
            case 0x20: cout << "lload_2"; pc++; break;
            case 0x21: cout << "lload_3"; pc++; break;
            case 0x22: cout << "fload_0"; pc++; break;
            case 0x23: cout << "fload_1"; pc++; break;
            case 0x24: cout << "fload_2"; pc++; break;
            case 0x25: cout << "fload_3"; pc++; break;
            case 0x26: cout << "dload_0"; pc++; break;
            case 0x27: cout << "dload_1"; pc++; break;
            case 0x28: cout << "dload_2"; pc++; break;
            case 0x29: cout << "dload_3"; pc++; break;
            case 0x2A: cout << "aload_0"; pc++; break;
            case 0x2B: cout << "aload_1"; pc++; break;
            case 0x2C: cout << "aload_2"; pc++; break;
            case 0x2D: cout << "aload_3"; pc++; break;

            case 0x2E: cout << "iaload"; pc++; break;
            case 0x2F: cout << "laload"; pc++; break;
            case 0x30: cout << "faload"; pc++; break;
            case 0x31: cout << "daload"; pc++; break;
            case 0x32: cout << "aaload"; pc++; break;
            case 0x33: cout << "baload"; pc++; break;
            case 0x34: cout << "caload"; pc++; break;
            case 0x35: cout << "saload"; pc++; break;

            case 0x36: cout << "istore " << (int)code[pc+1]; pc += 2; break;
            case 0x37: cout << "lstore " << (int)code[pc+1]; pc += 2; break;
            case 0x38: cout << "fstore " << (int)code[pc+1]; pc += 2; break;
            case 0x39: cout << "dstore " << (int)code[pc+1]; pc += 2; break;
            case 0x3A: cout << "astore " << (int)code[pc+1]; pc += 2; break;

            case 0x3B: cout << "istore_0"; pc++; break;
            case 0x3C: cout << "istore_1"; pc++; break;
            case 0x3D: cout << "istore_2"; pc++; break;
            case 0x3E: cout << "istore_3"; pc++; break;
            case 0x3F: cout << "lstore_0"; pc++; break;
            case 0x40: cout << "lstore_1"; pc++; break;
            case 0x41: cout << "lstore_2"; pc++; break;
            case 0x42: cout << "lstore_3"; pc++; break;
            case 0x43: cout << "fstore_0"; pc++; break;
            case 0x44: cout << "fstore_1"; pc++; break;
            case 0x45: cout << "fstore_2"; pc++; break;
            case 0x46: cout << "fstore_3"; pc++; break;
            case 0x47: cout << "dstore_0"; pc++; break;
            case 0x48: cout << "dstore_1"; pc++; break;
            case 0x49: cout << "dstore_2"; pc++; break;
            case 0x4A: cout << "dstore_3"; pc++; break;
            case 0x4B: cout << "astore_0"; pc++; break;
            case 0x4C: cout << "astore_1"; pc++; break;
            case 0x4D: cout << "astore_2"; pc++; break;
            case 0x4E: cout << "astore_3"; pc++; break;

            case 0x4F: cout << "iastore"; pc++; break;
            case 0x50: cout << "lastore"; pc++; break;
            case 0x51: cout << "fastore"; pc++; break;
            case 0x52: cout << "dastore"; pc++; break;
            case 0x53: cout << "aastore"; pc++; break;
            case 0x54: cout << "bastore"; pc++; break;
            case 0x55: cout << "castore"; pc++; break;
            case 0x56: cout << "sastore"; pc++; break;

            case 0x57: cout << "pop";      pc++; break;
            case 0x58: cout << "pop2";     pc++; break;
            case 0x59: cout << "dup";      pc++; break;
            case 0x5A: cout << "dup_x1";   pc++; break;
            case 0x5B: cout << "dup_x2";   pc++; break;
            case 0x5C: cout << "dup2";     pc++; break;
            case 0x5D: cout << "dup2_x1";  pc++; break;
            case 0x5E: cout << "dup2_x2";  pc++; break;
            case 0x5F: cout << "swap";     pc++; break;

            case 0x60: cout << "iadd"; pc++; break;
            case 0x61: cout << "ladd"; pc++; break;
            case 0x62: cout << "fadd"; pc++; break;
            case 0x63: cout << "dadd"; pc++; break;
            case 0x64: cout << "isub"; pc++; break;
            case 0x65: cout << "lsub"; pc++; break;
            case 0x66: cout << "fsub"; pc++; break;
            case 0x67: cout << "dsub"; pc++; break;
            case 0x68: cout << "imul"; pc++; break;
            case 0x69: cout << "lmul"; pc++; break;
            case 0x6A: cout << "fmul"; pc++; break;
            case 0x6B: cout << "dmul"; pc++; break;
            case 0x6C: cout << "idiv"; pc++; break;
            case 0x6D: cout << "ldiv"; pc++; break;
            case 0x6E: cout << "fdiv"; pc++; break;
            case 0x6F: cout << "ddiv"; pc++; break;
            case 0x70: cout << "irem"; pc++; break;
            case 0x71: cout << "lrem"; pc++; break;
            case 0x72: cout << "frem"; pc++; break;
            case 0x73: cout << "drem"; pc++; break;
            case 0x74: cout << "ineg"; pc++; break;
            case 0x75: cout << "lneg"; pc++; break;
            case 0x76: cout << "fneg"; pc++; break;
            case 0x77: cout << "dneg"; pc++; break;
            case 0x78: cout << "ishl"; pc++; break;
            case 0x79: cout << "lshl"; pc++; break;
            case 0x7A: cout << "ishr"; pc++; break;
            case 0x7B: cout << "lshr"; pc++; break;
            case 0x7C: cout << "iushr"; pc++; break;
            case 0x7D: cout << "lushr"; pc++; break;
            case 0x7E: cout << "iand"; pc++; break;
            case 0x7F: cout << "land"; pc++; break;
            case 0x80: cout << "ior";  pc++; break;
            case 0x81: cout << "lor";  pc++; break;
            case 0x82: cout << "ixor"; pc++; break;
            case 0x83: cout << "lxor"; pc++; break;
            case 0x84:
                cout << "iinc " << (int)code[pc+1] << " " << (int)(int8_t)code[pc+2];
                pc += 3; break;

            case 0x85: cout << "i2l"; pc++; break;
            case 0x86: cout << "i2f"; pc++; break;
            case 0x87: cout << "i2d"; pc++; break;
            case 0x88: cout << "l2i"; pc++; break;
            case 0x89: cout << "l2f"; pc++; break;
            case 0x8A: cout << "l2d"; pc++; break;
            case 0x8B: cout << "f2i"; pc++; break;
            case 0x8C: cout << "f2l"; pc++; break;
            case 0x8D: cout << "f2d"; pc++; break;
            case 0x8E: cout << "d2i"; pc++; break;
            case 0x8F: cout << "d2l"; pc++; break;
            case 0x90: cout << "d2f"; pc++; break;
            case 0x91: cout << "i2b"; pc++; break;
            case 0x92: cout << "i2c"; pc++; break;
            case 0x93: cout << "i2s"; pc++; break;

            case 0x94: cout << "lcmp";  pc++; break;
            case 0x95: cout << "fcmpl"; pc++; break;
            case 0x96: cout << "fcmpg"; pc++; break;
            case 0x97: cout << "dcmpl"; pc++; break;
            case 0x98: cout << "dcmpg"; pc++; break;

            case 0x99: cout << "ifeq "      << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0x9A: cout << "ifne "      << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0x9B: cout << "iflt "      << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0x9C: cout << "ifge "      << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0x9D: cout << "ifgt "      << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0x9E: cout << "ifle "      << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0x9F: cout << "if_icmpeq " << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA0: cout << "if_icmpne " << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA1: cout << "if_icmplt " << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA2: cout << "if_icmpge " << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA3: cout << "if_icmpgt " << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA4: cout << "if_icmple " << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA5: cout << "if_acmpeq " << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA6: cout << "if_acmpne " << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA7: cout << "goto "      << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA8: cout << "jsr "       << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xA9: cout << "ret " << (int)code[pc+1]; pc += 2; break;

            case 0xAA: {
                size_t start = pc++;
                while (pc % 4 != 0) pc++;
                int32_t def  = readS4(code, pc); pc += 4;
                int32_t low  = readS4(code, pc); pc += 4;
                int32_t high = readS4(code, pc); pc += 4;
                cout << "tableswitch " << low << " to " << high << "\n";
                for (int32_t j = low; j <= high; j++) {
                    int32_t off = readS4(code, pc); pc += 4;
                    cout << "              " << j << ": " << (int)start + off << "\n";
                }
                cout << "              default: " << (int)start + def;
                break;
            }

            case 0xAB: {
                size_t start = pc++;
                while (pc % 4 != 0) pc++;
                int32_t def    = readS4(code, pc); pc += 4;
                int32_t npairs = readS4(code, pc); pc += 4;
                cout << "lookupswitch " << npairs << " pairs\n";
                for (int32_t j = 0; j < npairs; j++) {
                    int32_t match = readS4(code, pc); pc += 4;
                    int32_t off   = readS4(code, pc); pc += 4;
                    cout << "              " << match << ": " << (int)start + off << "\n";
                }
                cout << "              default: " << (int)start + def;
                break;
            }

            case 0xAC: cout << "ireturn"; pc++; break;
            case 0xAD: cout << "lreturn"; pc++; break;
            case 0xAE: cout << "freturn"; pc++; break;
            case 0xAF: cout << "dreturn"; pc++; break;
            case 0xB0: cout << "areturn"; pc++; break;
            case 0xB1: cout << "return";  pc++; break;

            case 0xB2: { u2 idx = readU2(code, pc+1);
                cout << "getstatic #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xB3: { u2 idx = readU2(code, pc+1);
                cout << "putstatic #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xB4: { u2 idx = readU2(code, pc+1);
                cout << "getfield #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xB5: { u2 idx = readU2(code, pc+1);
                cout << "putfield #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xB6: { u2 idx = readU2(code, pc+1);
                cout << "invokevirtual #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xB7: { u2 idx = readU2(code, pc+1);
                cout << "invokespecial #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xB8: { u2 idx = readU2(code, pc+1);
                cout << "invokestatic #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xB9: { u2 idx = readU2(code, pc+1);
                cout << "invokeinterface #" << idx << ", " << (int)code[pc+3]
                     << "   // " << cpEntryComment(cls, idx);
                pc += 5; break; }
            case 0xBA: { u2 idx = readU2(code, pc+1);
                cout << "invokedynamic #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 5; break; }

            case 0xBB: { u2 idx = readU2(code, pc+1);
                cout << "new #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xBC: {
                static const char *types[] = {nullptr,nullptr,nullptr,nullptr,
                    "boolean","char","float","double","byte","short","int","long"};
                int t = code[pc+1];
                cout << "newarray " << ((t >= 4 && t <= 11) ? types[t] : "?");
                pc += 2; break; }
            case 0xBD: { u2 idx = readU2(code, pc+1);
                cout << "anewarray #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xBE: cout << "arraylength";  pc++; break;
            case 0xBF: cout << "athrow";       pc++; break;
            case 0xC0: { u2 idx = readU2(code, pc+1);
                cout << "checkcast #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xC1: { u2 idx = readU2(code, pc+1);
                cout << "instanceof #" << idx << "   // " << cpEntryComment(cls, idx);
                pc += 3; break; }
            case 0xC2: cout << "monitorenter"; pc++; break;
            case 0xC3: cout << "monitorexit";  pc++; break;

            case 0xC4: {
                u1 wop = code[pc+1];
                switch (wop) {
                case 0x15: cout << "wide iload "  << readU2(code, pc+2); break;
                case 0x16: cout << "wide lload "  << readU2(code, pc+2); break;
                case 0x17: cout << "wide fload "  << readU2(code, pc+2); break;
                case 0x18: cout << "wide dload "  << readU2(code, pc+2); break;
                case 0x19: cout << "wide aload "  << readU2(code, pc+2); break;
                case 0x36: cout << "wide istore " << readU2(code, pc+2); break;
                case 0x37: cout << "wide lstore " << readU2(code, pc+2); break;
                case 0x38: cout << "wide fstore " << readU2(code, pc+2); break;
                case 0x39: cout << "wide dstore " << readU2(code, pc+2); break;
                case 0x3A: cout << "wide astore " << readU2(code, pc+2); break;
                case 0xA9: cout << "wide ret "    << readU2(code, pc+2); break;
                case 0x84:
                    cout << "wide iinc " << readU2(code, pc+2)
                         << " " << (int16_t)readU2(code, pc+4);
                    pc += 6; break;
                default: cout << "wide ???"; break;
                }
                if (wop != 0x84) pc += 4;
                break;
            }

            case 0xC5: { u2 idx = readU2(code, pc+1);
                cout << "multianewarray #" << idx << " " << (int)code[pc+3]
                     << "   // " << cpEntryComment(cls, idx);
                pc += 4; break; }
            case 0xC6: cout << "ifnull "    << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xC7: cout << "ifnonnull " << (int)pc + readS2(code, pc+1); pc += 3; break;
            case 0xC8: cout << "goto_w "    << (int)pc + readS4(code, pc+1); pc += 5; break;
            case 0xC9: cout << "jsr_w "     << (int)pc + readS4(code, pc+1); pc += 5; break;

            default:
                cout << "unknown (0x" << std::hex << (int)op << std::dec << ")";
                pc++;
                break;
            }
            cout << "\n";
        }
    }
}

using namespace std;

Exibidor::Exibidor(const std::string &filename) : filename(filename)
{
    Parser parser(filename);
    classInfo = parser.parse();
}

Exibidor::~Exibidor() = default;

void Exibidor::display()
{
    cout << "Exibindo informações do arquivo " << filename << endl;

    cout << "------------------------------------------------------" << endl;
    cout << " Informações Gerais" << endl;
    cout << "------------------------------------------------------" << endl;
    cout << "Magic Number    : 0x" << hex << classInfo.magic_number << dec << "\n";
    cout << "Minor Version   : " << classInfo.minor_version << "\n";
    cout << "Major Version   : " << classInfo.major_version << "\n";
    cout << "Access Flags    : " << getClassAccessFlagsString(classInfo.access_flags) << "\n";
    cout << "This Class      : #" << classInfo.this_class
         << "  // " << classNameFromConstantPool(classInfo, classInfo.this_class) << "\n";
    cout << "Super Class     : #" << classInfo.super_class << "  // ";
    if (classInfo.super_class != 0)
        cout << classNameFromConstantPool(classInfo, classInfo.super_class);
    else
        cout << "(nenhuma)";
    cout << "\n";
    cout << "Interfaces Count: " << classInfo.interfaces_count << "\n";
    cout << "Fields Count    : " << classInfo.fields_count << "\n";
    cout << "Methods Count   : " << classInfo.methods_count << "\n";
    cout << "Constant Pool   : " << classInfo.constant_pool_count << " entradas\n";
    cout << "Attributes Count: " << classInfo.attributes_count << "\n";
    cout << "------------------------------------------------" << endl;
    constantPoolDisplay();

    if (classInfo.interfaces_count > 0) {
        cout << "------------------------------------------------------" << endl;
        cout << " Interfaces (" << classInfo.interfaces_count << ")" << endl;
        cout << "------------------------------------------------------" << endl;
        for (u2 i = 0; i < classInfo.interfaces_count; ++i)
            cout << "  [" << i + 1 << "] "
                 << classNameFromConstantPool(classInfo, classInfo.interfaces[i]) << "\n";
    }

    methodsDisplay();
    fieldsDisplay();
    attributesDisplay();
}

void Exibidor::constantPoolDisplay()
{
    using namespace std;

    cout << "------------------------------------------------------" << endl;
    cout << " Constant Pool (" << classInfo.constant_pool_count - 1 << " entradas)" << endl;
    cout << "------------------------------------------------------" << endl;

    for (u2 i = 1; i < classInfo.constant_pool_count; ++i) {
        const cp_info &entry = classInfo.constant_pool[i];

        if (entry.tag == 0) continue;

        cout << "  #" << left << setw(4) << i << " = ";

        string comment = cpEntryComment(classInfo, i);

        switch (entry.tag) {
        case CONSTANT_Class:
            cout << left << setw(19) << "Class"
                 << " #" << entry.container.Class.name_index;
            break;
        case CONSTANT_Fieldref:
            cout << left << setw(19) << "Fieldref"
                 << " #" << entry.container.Fieldref.class_index
                 << ".#" << entry.container.Fieldref.name_and_type_index;
            break;
        case CONSTANT_Methodref:
            cout << left << setw(19) << "Methodref"
                 << " #" << entry.container.Methodref.class_index
                 << ".#" << entry.container.Methodref.name_and_type_index;
            break;
        case CONSTANT_InterfaceMethodref:
            cout << left << setw(19) << "InterfaceMethodref"
                 << " #" << entry.container.InterfaceMethodref.class_index
                 << ".#" << entry.container.InterfaceMethodref.name_and_type_index;
            break;
        case CONSTANT_String:
            cout << left << setw(19) << "String"
                 << " #" << entry.container.String.string_index;
            break;
        case CONSTANT_Integer: {
            int32_t val = static_cast<int32_t>(entry.container.Integer.bytes);
            cout << left << setw(19) << "Integer" << " " << val;
            comment = to_string(val);
            break;
        }
        case CONSTANT_Float: {
            float val;
            u4 bits = entry.container.Float.bytes;
            memcpy(&val, &bits, sizeof(float));
            cout << left << setw(19) << "Float" << " " << val << "f";
            comment = to_string(val) + "f";
            break;
        }
        case CONSTANT_Long: {
            int64_t val = (int64_t)(((uint64_t)entry.container.Long.high_bytes << 32)
                                    | entry.container.Long.low_bytes);
            cout << left << setw(19) << "Long" << " " << val << "l";
            comment = to_string(val) + "l";
            break;
        }
        case CONSTANT_Double: {
            uint64_t bits = ((uint64_t)entry.container.Double.high_bytes << 32)
                            | entry.container.Double.low_bytes;
            double val;
            memcpy(&val, &bits, sizeof(double));
            cout << left << setw(19) << "Double" << " " << val;
            comment = to_string(val);
            break;
        }
        case CONSTANT_NameAndType:
            cout << left << setw(19) << "NameAndType"
                 << " #" << entry.container.NameAndType.name_index
                 << ".#" << entry.container.NameAndType.descriptor_index;
            break;
        case CONSTANT_Utf8:
            cout << left << setw(19) << "Utf8" << " " << entry.utf8_str;
            break;
        case CONSTANT_MethodHandle:
            cout << left << setw(19) << "MethodHandle"
                 << " " << static_cast<int>(entry.container.MethodHandle.reference_kind)
                 << " #" << entry.container.MethodHandle.reference_index;
            break;
        case CONSTANT_MethodType:
            cout << left << setw(19) << "MethodType"
                 << " #" << entry.container.MethodType.descriptor_index;
            break;
        case CONSTANT_InvokeDynamic:
            cout << left << setw(19) << "InvokeDynamic"
                 << " #" << entry.container.InvokeDynamic.bootstrap_method_attr_index
                 << ".#" << entry.container.InvokeDynamic.name_and_type_index;
            break;
        default:
            cout << "Unknown (tag=" << static_cast<int>(entry.tag) << ")";
            break;
        }

        if (!comment.empty()) {
            cout << "   // " << comment;
        }
        cout << "\n";
    }
}

void Exibidor::fieldsDisplay()
{
    cout << "------------------------------------------------------" << endl;
    cout << " Campos (" << classInfo.fields_count << ")" << endl;
    cout << "------------------------------------------------------" << endl;

    for (u2 i = 0; i < classInfo.fields_count; ++i) {
        const field_info &field = classInfo.fields.at(i);
        cout << "  [" << i + 1 << "] "
             << getFieldAccessFlagsString(field.access_flags) << " "
             << formatFieldType(utf8FromConstantPool(classInfo, field.descriptor_index)) << " "
             << utf8FromConstantPool(classInfo, field.name_index) << "\n";

        for (u2 j = 0; j < field.attributes_count; ++j) {
            const attribute_info &attr = field.attributes[j];
            string aname = utf8FromConstantPool(classInfo, attr.attribute_name_index);
            if (aname == "ConstantValue") {
                u2 idx = readU2(attr.info, 0);
                cout << "      ConstantValue: #" << idx
                     << "   // " << cpEntryComment(classInfo, idx) << "\n";
            } else if (aname == "Signature") {
                u2 idx = readU2(attr.info, 0);
                cout << "      Signature: " << utf8FromConstantPool(classInfo, idx) << "\n";
            } else if (aname == "Synthetic") {
                cout << "      Synthetic\n";
            } else if (aname == "Deprecated") {
                cout << "      Deprecated\n";
            }
        }
    }
}

void Exibidor::methodsDisplay()
{
    cout << "------------------------------------------------------" << endl;
    cout << " Metodos (" << classInfo.methods_count << ")" << endl;
    cout << "------------------------------------------------------" << endl;

    for (u2 i = 0; i < classInfo.methods_count; ++i)
    {
        const method_info &method = classInfo.methods.at(i);
        std::pair<string, string> signature = getMethodSignature(method);
        string methodName = utf8FromConstantPool(classInfo, method.name_index);

        cout << "  [" << i + 1 << "] ";

        if (methodName == "<clinit>") {
            cout << "static { }";
        } else {
            if (methodName == "<init>")
                methodName = classNameFromConstantPool(classInfo, classInfo.this_class);
            cout << getMethodAccessFlagsString(method.access_flags) << " " << signature.first
                 << " " << methodName << "(" << signature.second << ")";
        }
        cout << "\n";

        for (u2 j = 0; j < method.attributes_count; ++j) {
            if (method.attributes[j].code_data) {
                const code_attribute &ca = *method.attributes[j].code_data;
                cout << "      max_stack=" << ca.max_stack
                     << "  max_locals=" << ca.max_locals
                     << "  code_length=" << ca.code_length << "\n";
                disassembleCode(ca.code, ca.code_length, classInfo);

                if (ca.exception_table_length > 0) {
                    cout << "      Exception table:\n";
                    cout << "         from    to  target type\n";
                    for (u2 k = 0; k < ca.exception_table_length; ++k) {
                        const auto &exc = ca.exception_table[k];
                        cout << "         " << setw(4) << exc.start_pc
                             << "  " << setw(4) << exc.end_pc
                             << "  " << setw(4) << exc.handler_pc << "   ";
                        
                        if (exc.catch_type == 0) {
                            cout << "any\n"; // Se for 0, trata-se de um bloco finally (captura qualquer coisa)
                        } else {
                            cout << "Class " << classNameFromConstantPool(classInfo, exc.catch_type) << "\n";
                        }
                    }
                }
            }
        }
    }
}

std::pair<string, string> Exibidor::getMethodSignature(const method_info &method)
{
    string descriptor = utf8FromConstantPool(classInfo, method.descriptor_index);
    return {formatMethodReturnType(descriptor), formatMethodParameters(descriptor)};
}

void Exibidor::attributesDisplay()
{
    cout << "------------------------------------------------------" << endl;
    cout << " Atributos da Classe (" << classInfo.attributes_count << ")" << endl;
    cout << "------------------------------------------------------" << endl;

    for (u2 i = 0; i < classInfo.attributes_count; ++i) {
        const attribute_info &attr = classInfo.attributes[i];
        string name = utf8FromConstantPool(classInfo, attr.attribute_name_index);
        const auto &data = attr.info;

        cout << "  [" << i + 1 << "] " << name;

        if (name == "SourceFile") {
            u2 idx = readU2(data, 0);
            cout << ": " << utf8FromConstantPool(classInfo, idx);
        } else if (name == "Signature") {
            u2 idx = readU2(data, 0);
            cout << ": " << utf8FromConstantPool(classInfo, idx);
        } else if (name == "Deprecated" || name == "Synthetic") {
        } else if (name == "InnerClasses") {
            u2 count = readU2(data, 0);
            cout << " (" << count << ")";
            size_t pos = 2;
            for (u2 j = 0; j < count; ++j) {
                u2 inner = readU2(data, pos); pos += 2;
                u2 outer = readU2(data, pos); pos += 2;
                u2 iname = readU2(data, pos); pos += 2;
                u2 flags = readU2(data, pos); pos += 2;
                cout << "\n    ";
                if (inner != 0) cout << cpEntryComment(classInfo, inner);
                if (outer != 0) cout << " em " << cpEntryComment(classInfo, outer);
                if (iname != 0) cout << " como " << utf8FromConstantPool(classInfo, iname);
                cout << "  [" << getClassAccessFlagsString(flags) << "]";
            }
        } else if (name == "EnclosingMethod") {
            u2 class_idx  = readU2(data, 0);
            u2 method_idx = readU2(data, 2);
            cout << ": " << cpEntryComment(classInfo, class_idx);
            if (method_idx != 0) cout << "." << nameAndTypeStr(classInfo, method_idx);
        } else if (name == "BootstrapMethods") {
            u2 count = readU2(data, 0);
            cout << " (" << count << ")";
            size_t pos = 2;
            for (u2 j = 0; j < count; ++j) {
                u2 ref   = readU2(data, pos); pos += 2;
                u2 nargs = readU2(data, pos); pos += 2;
                cout << "\n    #" << ref << " args:";
                for (u2 k = 0; k < nargs; ++k) {
                    u2 arg = readU2(data, pos); pos += 2;
                    cout << " #" << arg;
                }
            }
        } else {
            cout << " (" << attr.attribute_length << " bytes)";
        }
        cout << "\n";
    }
}
