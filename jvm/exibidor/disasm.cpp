#include "disasm.hpp"
#include "bytecode_utils.hpp"
#include "cp_utils.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>

using namespace std;

/**
 * @brief Valida os limites de leitura do vetor de bytecodes.
 * Previne falhas de segmentação (Segfault) e Buffer Overflow caso o bytecode 
 * tente instruir a leitura de argumentos que ultrapassem o tamanho do método.
 */
static void requireBytes(size_t pos, size_t n, size_t code_length)
{
    if (n > code_length || pos > code_length - n)
        throw std::runtime_error("disassembler: bytecode truncado no offset "
                                 + std::to_string(pos));
}

/**
 * @brief O loop principal do interpretador de Bytecode (Disassembler).
 * 
 * Percorre o array de bytes instrução por instrução, mantendo um Program Counter (pc).
 * Decodifica o opcode (switch principal), consome a quantidade correta de operandos e 
 * formata o assembly do Java com as devidas referências e saltos calculados matematicamente.
 */
void disassembleCode(const std::vector<u1> &code, u4 code_length, const class_info &cls)
{
    size_t pc = 0;
    while (pc < code_length) {
        u1 op = code[pc];
        cout << "      " << setw(4) << pc << ": ";

        switch (op) {
        case 0x00: cout << "nop";         pc++; break;
        case 0x01: cout << "aconst_null"; pc++; break;
        case 0x02: cout << "iconst_m1";   pc++; break;
        case 0x03: cout << "iconst_0";    pc++; break;
        case 0x04: cout << "iconst_1";    pc++; break;
        case 0x05: cout << "iconst_2";    pc++; break;
        case 0x06: cout << "iconst_3";    pc++; break;
        case 0x07: cout << "iconst_4";    pc++; break;
        case 0x08: cout << "iconst_5";    pc++; break;
        case 0x09: cout << "lconst_0";    pc++; break;
        case 0x0A: cout << "lconst_1";    pc++; break;
        case 0x0B: cout << "fconst_0";    pc++; break;
        case 0x0C: cout << "fconst_1";    pc++; break;
        case 0x0D: cout << "fconst_2";    pc++; break;
        case 0x0E: cout << "dconst_0";    pc++; break;
        case 0x0F: cout << "dconst_1";    pc++; break;

        case 0x10: requireBytes(pc, 2, code_length);
            cout << "bipush " << (int)(int8_t)code[pc + 1]; pc += 2; break;
        case 0x11: requireBytes(pc, 3, code_length);
            cout << "sipush " << (int)readS2(code, pc + 1); pc += 3; break;

        case 0x12: { requireBytes(pc, 2, code_length);
            u1 idx = code[pc + 1];
            cout << "ldc #" << (int)idx << "   // " << cpEntryComment(cls, idx);
            pc += 2; break; }
        case 0x13: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "ldc_w #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0x14: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "ldc2_w #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }

        case 0x15: requireBytes(pc, 2, code_length);
            cout << "iload "  << (int)code[pc + 1]; pc += 2; break;
        case 0x16: requireBytes(pc, 2, code_length);
            cout << "lload "  << (int)code[pc + 1]; pc += 2; break;
        case 0x17: requireBytes(pc, 2, code_length);
            cout << "fload "  << (int)code[pc + 1]; pc += 2; break;
        case 0x18: requireBytes(pc, 2, code_length);
            cout << "dload "  << (int)code[pc + 1]; pc += 2; break;
        case 0x19: requireBytes(pc, 2, code_length);
            cout << "aload "  << (int)code[pc + 1]; pc += 2; break;

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

        case 0x36: requireBytes(pc, 2, code_length);
            cout << "istore " << (int)code[pc + 1]; pc += 2; break;
        case 0x37: requireBytes(pc, 2, code_length);
            cout << "lstore " << (int)code[pc + 1]; pc += 2; break;
        case 0x38: requireBytes(pc, 2, code_length);
            cout << "fstore " << (int)code[pc + 1]; pc += 2; break;
        case 0x39: requireBytes(pc, 2, code_length);
            cout << "dstore " << (int)code[pc + 1]; pc += 2; break;
        case 0x3A: requireBytes(pc, 2, code_length);
            cout << "astore " << (int)code[pc + 1]; pc += 2; break;

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

        case 0x57: cout << "pop";     pc++; break;
        case 0x58: cout << "pop2";    pc++; break;
        case 0x59: cout << "dup";     pc++; break;
        case 0x5A: cout << "dup_x1";  pc++; break;
        case 0x5B: cout << "dup_x2";  pc++; break;
        case 0x5C: cout << "dup2";    pc++; break;
        case 0x5D: cout << "dup2_x1"; pc++; break;
        case 0x5E: cout << "dup2_x2"; pc++; break;
        case 0x5F: cout << "swap";    pc++; break;

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
        case 0x78: cout << "ishl";  pc++; break;
        case 0x79: cout << "lshl";  pc++; break;
        case 0x7A: cout << "ishr";  pc++; break;
        case 0x7B: cout << "lshr";  pc++; break;
        case 0x7C: cout << "iushr"; pc++; break;
        case 0x7D: cout << "lushr"; pc++; break;
        case 0x7E: cout << "iand";  pc++; break;
        case 0x7F: cout << "land";  pc++; break;
        case 0x80: cout << "ior";   pc++; break;
        case 0x81: cout << "lor";   pc++; break;
        case 0x82: cout << "ixor";  pc++; break;
        case 0x83: cout << "lxor";  pc++; break;
        case 0x84: requireBytes(pc, 3, code_length);
            cout << "iinc " << (int)code[pc + 1] << " " << (int)(int8_t)code[pc + 2];
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

        case 0x99: requireBytes(pc, 3, code_length);
            cout << "ifeq "      << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0x9A: requireBytes(pc, 3, code_length);
            cout << "ifne "      << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0x9B: requireBytes(pc, 3, code_length);
            cout << "iflt "      << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0x9C: requireBytes(pc, 3, code_length);
            cout << "ifge "      << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0x9D: requireBytes(pc, 3, code_length);
            cout << "ifgt "      << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0x9E: requireBytes(pc, 3, code_length);
            cout << "ifle "      << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0x9F: requireBytes(pc, 3, code_length);
            cout << "if_icmpeq " << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA0: requireBytes(pc, 3, code_length);
            cout << "if_icmpne " << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA1: requireBytes(pc, 3, code_length);
            cout << "if_icmplt " << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA2: requireBytes(pc, 3, code_length);
            cout << "if_icmpge " << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA3: requireBytes(pc, 3, code_length);
            cout << "if_icmpgt " << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA4: requireBytes(pc, 3, code_length);
            cout << "if_icmple " << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA5: requireBytes(pc, 3, code_length);
            cout << "if_acmpeq " << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA6: requireBytes(pc, 3, code_length);
            cout << "if_acmpne " << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA7: requireBytes(pc, 3, code_length);
            cout << "goto "      << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA8: requireBytes(pc, 3, code_length);
            cout << "jsr "       << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xA9: requireBytes(pc, 2, code_length);
            cout << "ret " << (int)code[pc + 1]; pc += 2; break;

        case 0xAA: {
            size_t start = pc++;
            while (pc % 4 != 0) pc++;
            // cabeçalho: default (4) + low (4) + high (4) = 12 bytes
            requireBytes(pc, 12, code_length);
            int32_t def  = readS4(code, pc); pc += 4;
            int32_t low  = readS4(code, pc); pc += 4;
            int32_t high = readS4(code, pc); pc += 4;
            if (high < low)
                throw std::runtime_error("tableswitch: high < low (bytecode inválido)");
            // A spec JVM não define limite explícito de entradas, mas 65536
            // é um teto seguro: nenhum método válido excede 65535 bytes.
            int64_t count = static_cast<int64_t>(high) - low + 1;
            if (count > 65536)
                throw std::runtime_error("tableswitch: número de entradas excede 65536");
            requireBytes(pc, static_cast<size_t>(count) * 4, code_length);
            cout << "tableswitch " << low << " to " << high << "\n";
            for (int64_t j = low; j <= high; j++) {
                int32_t off = readS4(code, pc); pc += 4;
                cout << "              " << j << ": " << static_cast<int64_t>(start) + off << "\n";
            }
            cout << "              default: " << static_cast<int64_t>(start) + def;
            break;
        }

        case 0xAB: {
            size_t start = pc++;
            while (pc % 4 != 0) pc++;
            // cabeçalho: default (4) + npairs (4) = 8 bytes
            requireBytes(pc, 8, code_length);
            int32_t def    = readS4(code, pc); pc += 4;
            int32_t npairs = readS4(code, pc); pc += 4;
            if (npairs < 0)
                throw std::runtime_error("lookupswitch: npairs negativo (bytecode inválido)");
            if (npairs > 65536)
                throw std::runtime_error("lookupswitch: número de pares excede 65536");
            requireBytes(pc, static_cast<size_t>(npairs) * 8, code_length);
            cout << "lookupswitch " << npairs << " pairs\n";
            for (int32_t j = 0; j < npairs; j++) {
                int32_t match = readS4(code, pc); pc += 4;
                int32_t off   = readS4(code, pc); pc += 4;
                cout << "              " << match << ": " << static_cast<int64_t>(start) + off << "\n";
            }
            cout << "              default: " << static_cast<int64_t>(start) + def;
            break;
        }

        case 0xAC: cout << "ireturn"; pc++; break;
        case 0xAD: cout << "lreturn"; pc++; break;
        case 0xAE: cout << "freturn"; pc++; break;
        case 0xAF: cout << "dreturn"; pc++; break;
        case 0xB0: cout << "areturn"; pc++; break;
        case 0xB1: cout << "return";  pc++; break;

        case 0xB2: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "getstatic #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xB3: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "putstatic #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xB4: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "getfield #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xB5: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "putfield #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xB6: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "invokevirtual #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xB7: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "invokespecial #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xB8: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "invokestatic #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xB9: { requireBytes(pc, 5, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "invokeinterface #" << idx << ", " << (int)code[pc + 3]
                 << "   // " << cpEntryComment(cls, idx);
            pc += 5; break; }
        case 0xBA: { requireBytes(pc, 5, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "invokedynamic #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 5; break; }

        case 0xBB: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "new #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xBC: { requireBytes(pc, 2, code_length);
            static const char *types[] = {nullptr,nullptr,nullptr,nullptr,
                "boolean","char","float","double","byte","short","int","long"};
            int t = code[pc + 1];
            cout << "newarray " << ((t >= 4 && t <= 11) ? types[t] : "?");
            pc += 2; break; }
        case 0xBD: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "anewarray #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xBE: cout << "arraylength"; pc++; break;
        case 0xBF: cout << "athrow";      pc++; break;
        case 0xC0: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "checkcast #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xC1: { requireBytes(pc, 3, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "instanceof #" << idx << "   // " << cpEntryComment(cls, idx);
            pc += 3; break; }
        case 0xC2: cout << "monitorenter"; pc++; break;
        case 0xC3: cout << "monitorexit";  pc++; break;

        case 0xC4: { requireBytes(pc, 2, code_length);
            u1 wop = code[pc + 1];
            if (wop == 0x84) {
                requireBytes(pc, 6, code_length);
                cout << "wide iinc " << readU2(code, pc + 2)
                     << " " << static_cast<int16_t>(readU2(code, pc + 4));
                pc += 6;
            } else {
                requireBytes(pc, 4, code_length);
                switch (wop) {
                case 0x15: cout << "wide iload "  << readU2(code, pc + 2); break;
                case 0x16: cout << "wide lload "  << readU2(code, pc + 2); break;
                case 0x17: cout << "wide fload "  << readU2(code, pc + 2); break;
                case 0x18: cout << "wide dload "  << readU2(code, pc + 2); break;
                case 0x19: cout << "wide aload "  << readU2(code, pc + 2); break;
                case 0x36: cout << "wide istore " << readU2(code, pc + 2); break;
                case 0x37: cout << "wide lstore " << readU2(code, pc + 2); break;
                case 0x38: cout << "wide fstore " << readU2(code, pc + 2); break;
                case 0x39: cout << "wide dstore " << readU2(code, pc + 2); break;
                case 0x3A: cout << "wide astore " << readU2(code, pc + 2); break;
                case 0xA9: cout << "wide ret "    << readU2(code, pc + 2); break;
                default:   cout << "wide ???"; break;
                }
                pc += 4;
            }
            break;
        }

        case 0xC5: { requireBytes(pc, 4, code_length);
            u2 idx = readU2(code, pc + 1);
            cout << "multianewarray #" << idx << " " << (int)code[pc + 3]
                 << "   // " << cpEntryComment(cls, idx);
            pc += 4; break; }
        case 0xC6: requireBytes(pc, 3, code_length);
            cout << "ifnull "    << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xC7: requireBytes(pc, 3, code_length);
            cout << "ifnonnull " << static_cast<int64_t>(pc) + readS2(code, pc + 1); pc += 3; break;
        case 0xC8: requireBytes(pc, 5, code_length);
            cout << "goto_w "    << static_cast<int64_t>(pc) + readS4(code, pc + 1); pc += 5; break;
        case 0xC9: requireBytes(pc, 5, code_length);
            cout << "jsr_w "     << static_cast<int64_t>(pc) + readS4(code, pc + 1); pc += 5; break;

        default:
            cout << "unknown (0x" << std::hex << (int)op << std::dec << ")";
            pc++;
            break;
        }
        cout << "\n";
    }
}
