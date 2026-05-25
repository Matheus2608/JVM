#include "exibidor.hpp"
#include "bytecode_utils.hpp"
#include "cp_utils.hpp"
#include "descriptor.hpp"
#include "disasm.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>

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
            int64_t val = static_cast<int64_t>(
                (static_cast<uint64_t>(entry.container.Long.high_bytes) << 32) |
                 entry.container.Long.low_bytes);
            cout << left << setw(19) << "Long" << " " << val << "l";
            comment = to_string(val) + "l";
            break;
        }
        case CONSTANT_Double: {
            uint64_t bits = (static_cast<uint64_t>(entry.container.Double.high_bytes) << 32) |
                             entry.container.Double.low_bytes;
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
        case CONSTANT_Module:
            cout << left << setw(19) << "Module"
                 << " #" << entry.container.Class.name_index;
            break;
        case CONSTANT_Package:
            cout << left << setw(19) << "Package"
                 << " #" << entry.container.Class.name_index;
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
            else if (aname == "RuntimeVisibleAnnotations" || aname == "RuntimeInvisibleAnnotations") {
                const vector<u1>& dados = attr.info;
                if (dados.size() >= 2) {
                    u2 num_annotations = readU2(dados, 0);
                    cout << "      " << aname << ": " << num_annotations << " anotacao(oes) encontrada(s).\n";
                    
                    cout << "        Dados brutos (Hex): ";
                    for(size_t k = 2; k < dados.size(); ++k) {
                        cout << hex << uppercase << setfill('0') << setw(2) 
                             << static_cast<int>(dados[k]) << " " << dec;
                    }
                    cout << "\n";
                }
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
            const attribute_info &attr = method.attributes[j];
            string attr_name = utf8FromConstantPool(classInfo, attr.attribute_name_index);
            if (attr.code_data) {
                const code_attribute &ca = *attr.code_data;
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
                        if (exc.catch_type == 0)
                            cout << "any\n";
                        else
                            cout << "Class " << classNameFromConstantPool(classInfo, exc.catch_type) << "\n";
                    }
                }

                if (ca.attributes_count > 0) {
                    for (u2 a = 0; a < ca.attributes_count; ++a) {
                        const attribute_info &inner_attr = ca.attributes[a];
                        string inner_attr_name = utf8FromConstantPool(classInfo, inner_attr.attribute_name_index);
                        
                        const vector<u1>& dados = inner_attr.info;
                        if (inner_attr_name == "LineNumberTable" && dados.size() >= 2) {
                        
                            u2 lnt_length = (dados[0] << 8) | dados[1];
                            
                            cout << "      LineNumberTable:\n";
                            cout << "        Start PC      Line Number\n";
                                                        
                            for (u2 l = 0; l < lnt_length; ++l) {
                                u2 offset = 2 + (l * 4);
                                if (offset + 3U < dados.size()) {
                                    u2 start_pc    = (dados[offset] << 8) | dados[offset + 1];
                                    u2 line_number = (dados[offset + 2] << 8) | dados[offset + 3];
                                    cout << "        " << setw(8) << start_pc << "      " << line_number << "\n";
                                }
                            }
                        } 
                        else if (inner_attr_name == "LocalVariableTable" && dados.size() >= 2) {
                            
                            u2 lvt_length = (dados[0] << 8) | dados[1];
                            
                            cout << "      LocalVariableTable:\n";
                            cout << "        Start  Length  Slot  Name            Signature\n";
                                                       
                            for (u2 l = 0; l < lvt_length; ++l) {
                                u2 offset = 2 + (l * 10);
                                if (offset + 9U < dados.size()) {
                                    u2 start_pc         = (dados[offset] << 8) | dados[offset + 1];
                                    u2 length           = (dados[offset + 2] << 8) | dados[offset + 3];
                                    u2 name_index       = (dados[offset + 4] << 8) | dados[offset + 5];
                                    u2 descriptor_index = (dados[offset + 6] << 8) | dados[offset + 7];
                                    u2 slot_index       = (dados[offset + 8] << 8) | dados[offset + 9];

                                    cout << "        " << setw(5) << start_pc
                                         << "  " << setw(6) << length
                                         << "  " << setw(4) << slot_index
                                         << "  " << setw(14) << utf8FromConstantPool(classInfo, name_index)
                                         << "  " << utf8FromConstantPool(classInfo, descriptor_index) << "\n";
                                }
                            }
                        }
                    }
                }
            }
            else if (attr_name == "Exceptions") {
                const vector<u1>& dados = attr.info;
                if (dados.size() >= 2) {
                    u2 number_of_exceptions = (dados[0] << 8) | dados[1];
                    cout << "      Exceptions:\n";
                    for (u2 e = 0; e < number_of_exceptions; ++e) {
                        u2 offset = 2 + (e * 2);
                        if (offset + 1U < dados.size()) {
                            // Pega o índice e busca a string da classe de erro no Constant Pool
                            u2 exc_index = (dados[offset] << 8) | dados[offset + 1];
                            cout << "        throws " << classNameFromConstantPool(classInfo, exc_index) << "\n";
                        }
                    }
                }
            }
            else if (attr_name == "Signature") {
                const vector<u1>& dados = attr.info;
                if (dados.size() >= 2) {
                    u2 sig_index = (dados[0] << 8) | dados[1];
                    cout << "      Signature: " << utf8FromConstantPool(classInfo, sig_index) << "\n";
                }
            }
            else if (attr_name == "Deprecated") {
                cout << "      Deprecated: true\n";
            }
            else if (attr_name == "Synthetic") {
                cout << "      Synthetic: true\n";
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

        } 
        else if (name == "RuntimeVisibleAnnotations" || name == "RuntimeInvisibleAnnotations") {
            if (data.size() >= 2) {
                u2 num_annotations = readU2(data, 0);
                cout << " (" << num_annotations << " anotacao(oes))";
                cout << "\n    Dados brutos (Hex): ";
                for(size_t k = 2; k < data.size(); ++k) {
                    cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(data[k]) << " " << dec;
                }
            }
        } 
        else if (name == "NestHost") {
            if (data.size() >= 2) {
                u2 host_class_index = readU2(data, 0);
                cout << ": #" << host_class_index << " // " << cpEntryComment(classInfo, host_class_index);
            }
        } 
        else if (name == "NestMembers") {
            if (data.size() >= 2) {
                u2 number_of_classes = readU2(data, 0);
                cout << " (" << number_of_classes << " membros)";
                size_t pos = 2;
                for (u2 j = 0; j < number_of_classes; ++j) {
                    if (pos + 1 < data.size()) {
                        u2 class_index = readU2(data, pos); pos += 2;
                        cout << "\n    #" << class_index << " // " << cpEntryComment(classInfo, class_index);
                    }
                }
            }
        } 
        else if (name == "Module") {
            if (data.size() >= 6) {
                u2 module_name_index = readU2(data, 0);
                u2 module_flags = readU2(data, 2);
                u2 module_version_index = readU2(data, 4);
                cout << ":";
                cout << "\n    Name   : #" << module_name_index << " // " << cpEntryComment(classInfo, module_name_index);
                cout << "\n    Flags  : 0x" << hex << module_flags << dec;
                cout << "\n    Version: #" << module_version_index;
                if (module_version_index != 0) cout << " // " << cpEntryComment(classInfo, module_version_index);
                
                if (data.size() > 6) {
                    cout << "\n    Dados complementares (Hex): ";
                    for(size_t k = 6; k < data.size(); ++k) {
                        cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(data[k]) << " " << dec;
                    }
                }
            }
        } else {
            cout << " (" << attr.attribute_length << " bytes)";
        }
        cout << "\n";
    }
}
