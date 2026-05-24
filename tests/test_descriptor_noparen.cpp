// Verifica que formatMethodReturnType lança exceção quando o descriptor
// não contém ')'. Sem a correção, closeParen == npos (SIZE_MAX), e a
// expressão `npos + 1` sofre wrap-around para 0; a guarda `0 >= size()`
// é falsa para qualquer string não-vazia, então parseTypeDescriptor é
// chamado com position=0 e não lança — comportamento incorreto.

#include <cassert>
#include <stdexcept>
#include <iostream>
#include "descriptor.hpp"

static void test_sem_parentese_fechamento()
{
    // Descriptor sem ')' — deve lançar, não retornar silenciosamente
    bool threw = false;
    try {
        formatMethodReturnType("I");
    } catch (const std::runtime_error &) {
        threw = true;
    }
    assert(threw && "descriptor sem ')' deve lançar runtime_error");
}

static void test_parentese_no_fim_sem_tipo_retorno()
{
    // ')' existe mas não há nada após ele
    bool threw = false;
    try {
        formatMethodReturnType("()");
    } catch (const std::runtime_error &) {
        threw = true;
    }
    assert(threw && "descriptor com ')' no final sem tipo de retorno deve lançar");
}

static void test_string_vazia()
{
    bool threw = false;
    try {
        formatMethodReturnType("");
    } catch (const std::runtime_error &) {
        threw = true;
    }
    assert(threw && "descriptor vazio deve lançar runtime_error");
}

static void test_descriptor_valido_void()
{
    // ()V é válido: sem parâmetros, retorno void
    bool threw = false;
    std::string result;
    try {
        result = formatMethodReturnType("()V");
    } catch (const std::exception &) {
        threw = true;
    }
    assert(!threw && "descriptor valido nao deve lancar excecao");
    assert(result == "void" && "retorno de ()V deve ser 'void'");
}

static void test_descriptor_valido_int()
{
    std::string result = formatMethodReturnType("(II)I");
    assert(result == "int" && "retorno de (II)I deve ser 'int'");
}

static void test_descriptor_valido_objeto()
{
    std::string result = formatMethodReturnType("()Ljava/lang/String;");
    assert(result == "java/lang/String" && "retorno de objeto deve ser o nome da classe");
}

int main()
{
    test_sem_parentese_fechamento();
    test_parentese_no_fim_sem_tipo_retorno();
    test_string_vazia();
    test_descriptor_valido_void();
    test_descriptor_valido_int();
    test_descriptor_valido_objeto();

    std::cout << "test_descriptor_noparen: todos os testes passaram." << std::endl;
    return 0;
}
