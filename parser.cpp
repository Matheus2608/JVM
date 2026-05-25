#include "parser.hpp"

#include <stdexcept>

using namespace std;

Parser::Parser(const string &filename) : classInfo(), leitor(filename) {}

Parser::~Parser() = default;

class_info Parser::parse()
{
    parseMagic();
    parseMinorVersion();
    parseMajorVersion();
    parseConstantPoolCount();
    parseConstantPool();
    parseAccessFlags();
    parseThisClass();
    parseSuperClass();
    parseInterfacesCount();
    parseInterfaces();
    parseFieldsCount();
    parseFields();
    parseMethodsCount();
    parseMethods();
    parseAttributesCount();
    parseAttributes();
    return classInfo;
}

void Parser::parseMagic()
{
    u4 magic = leitor.readu4();
    if (magic != 0xCAFEBABE)
    {
        throw runtime_error("Arquivo Java invalido!");
    }
    classInfo.magic_number = magic;
}

void Parser::parseMinorVersion()
{
    classInfo.minor_version = leitor.readu2();
}

void Parser::parseMajorVersion()
{
    classInfo.major_version = leitor.readu2();
}

void Parser::parseConstantPoolCount()
{
    classInfo.constant_pool_count = leitor.readu2();
}

void Parser::parseConstantPool()
{
    classInfo.constant_pool.resize(classInfo.constant_pool_count);

    for (u2 i = 1; i < classInfo.constant_pool_count; ++i)
    {
        cp_info &entry = classInfo.constant_pool[i];
        entry.tag = leitor.readu1();

        switch (entry.tag)
        {
        case CONSTANT_Class:
            entry.container.Class.name_index = leitor.readu2();
            break;
        case CONSTANT_Module:
        case CONSTANT_Package:
            entry.container.Class.name_index = leitor.readu2();
            break;
        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
            entry.container.Fieldref.class_index = leitor.readu2();
            entry.container.Fieldref.name_and_type_index = leitor.readu2();
            break;
        case CONSTANT_String:
            entry.container.String.string_index = leitor.readu2();
            break;
        case CONSTANT_Integer:
        case CONSTANT_Float:
            entry.container.Integer.bytes = leitor.readu4();
            break;
        case CONSTANT_Long:
        case CONSTANT_Double:
            entry.container.Long.high_bytes = leitor.readu4();
            entry.container.Long.low_bytes = leitor.readu4();
            ++i;
            break;
        case CONSTANT_MethodHandle:
            entry.container.MethodHandle.reference_kind = leitor.readu1();
            entry.container.MethodHandle.reference_index = leitor.readu2();
            break;
        case CONSTANT_MethodType:
            entry.container.MethodType.descriptor_index = leitor.readu2();
            break;
        case CONSTANT_InvokeDynamic:
            entry.container.InvokeDynamic.bootstrap_method_attr_index = leitor.readu2();
            entry.container.InvokeDynamic.name_and_type_index = leitor.readu2();
            break;
        case CONSTANT_NameAndType:
            entry.container.NameAndType.name_index = leitor.readu2();
            entry.container.NameAndType.descriptor_index = leitor.readu2();
            break;
        case CONSTANT_Utf8:
        {
            u2 length = leitor.readu2();
            std::vector<u1> bytes = leitor.read_bytes(length);
            entry.utf8_str = std::string(reinterpret_cast<const char *>(bytes.data()), length);
            break;
        }
        default:
            throw runtime_error("Parser: constante da constant pool nao suportada: tag=" + to_string(entry.tag));
        }
    }
}

void Parser::parseAccessFlags()
{
    classInfo.access_flags = leitor.readu2();
}

void Parser::parseThisClass()
{
    classInfo.this_class = leitor.readu2();
}

void Parser::parseSuperClass()
{
    classInfo.super_class = leitor.readu2();
}

void Parser::parseInterfacesCount()
{
    classInfo.interfaces_count = leitor.readu2();
}

void Parser::parseInterfaces()
{
    classInfo.interfaces.resize(classInfo.interfaces_count);
    for (u2 i = 0; i < classInfo.interfaces_count; ++i)
    {
        classInfo.interfaces[i] = leitor.readu2();
    }
}

void Parser::parseFieldsCount()
{
    classInfo.fields_count = leitor.readu2();
}

void Parser::parseFields()
{
    classInfo.fields.resize(classInfo.fields_count);
    for (u2 i = 0; i < classInfo.fields_count; ++i)
    {
        field_info &field = classInfo.fields[i];
        field.access_flags = leitor.readu2();
        field.name_index = leitor.readu2();
        field.descriptor_index = leitor.readu2();
        field.attributes_count = leitor.readu2();
        field.attributes.resize(field.attributes_count);

        for (u2 j = 0; j < field.attributes_count; ++j)
        {
            attribute_info &attribute = field.attributes[j];
            attribute.attribute_name_index = leitor.readu2();
            attribute.attribute_length = leitor.readu4();
            attribute.info = leitor.read_bytes(attribute.attribute_length);
        }
    }
}

void Parser::parseMethodsCount()
{
    classInfo.methods_count = leitor.readu2();
}

void Parser::parseMethods()
{
    classInfo.methods.resize(classInfo.methods_count);
    for (u2 i = 0; i < classInfo.methods_count; ++i)
    {
        method_info &method = classInfo.methods[i];
        method.access_flags = leitor.readu2();
        method.name_index = leitor.readu2();
        method.descriptor_index = leitor.readu2();
        method.attributes_count = leitor.readu2();
        method.attributes.resize(method.attributes_count);

        for (u2 j = 0; method.attributes_count > j; ++j)
        {
            attribute_info &attribute = method.attributes[j];
            attribute.attribute_name_index = leitor.readu2();
            attribute.attribute_length = leitor.readu4();

            std::string attr_name = getUtf8Constant(attribute.attribute_name_index);

            if (attr_name == "Code") {
                attribute.code_data = std::make_shared<code_attribute>();
                attribute.code_data->attribute_name_index = attribute.attribute_name_index;
                attribute.code_data->attribute_length = attribute.attribute_length;
                
                attribute.code_data->max_stack = leitor.readu2();
                attribute.code_data->max_locals = leitor.readu2();

                attribute.code_data->code_length = leitor.readu4();
                
                attribute.code_data->code = leitor.read_bytes(attribute.code_data->code_length);
                
                attribute.code_data->exception_table_length = leitor.readu2();
                attribute.code_data->exception_table.resize(attribute.code_data->exception_table_length);
                for (u2 k = 0; k < attribute.code_data->exception_table_length; ++k) {
                    attribute.code_data->exception_table[k].start_pc = leitor.readu2();
                    attribute.code_data->exception_table[k].end_pc = leitor.readu2();
                    attribute.code_data->exception_table[k].handler_pc = leitor.readu2();
                    attribute.code_data->exception_table[k].catch_type = leitor.readu2();
                }
                
                attribute.code_data->attributes_count = leitor.readu2();
                attribute.code_data->attributes.resize(attribute.code_data->attributes_count);
                for (u2 k = 0; k < attribute.code_data->attributes_count; ++k) {
                    attribute.code_data->attributes[k].attribute_name_index = leitor.readu2();
                    attribute.code_data->attributes[k].attribute_length = leitor.readu4();
                    attribute.code_data->attributes[k].info = leitor.read_bytes(attribute.code_data->attributes[k].attribute_length);
                }
            } else {
                attribute.info = leitor.read_bytes(attribute.attribute_length);
            }
        }
    }
}

void Parser::parseAttributesCount()
{
    classInfo.attributes_count = leitor.readu2();
}

void Parser::parseAttributes()
{
    classInfo.attributes.resize(classInfo.attributes_count);
    for (u2 i = 0; i < classInfo.attributes_count; ++i)
    {
        attribute_info &attribute = classInfo.attributes[i];
        attribute.attribute_name_index = leitor.readu2();
        attribute.attribute_length = leitor.readu4();
        attribute.info = leitor.read_bytes(attribute.attribute_length);
    }
}

std::string Parser::getUtf8Constant(u2 index)
{
    if (index == 0 || index >= classInfo.constant_pool.size()) return "";
    
    const cp_info &entry = classInfo.constant_pool[index];
    if (entry.tag != CONSTANT_Utf8) return "";
    
    return entry.utf8_str;
}