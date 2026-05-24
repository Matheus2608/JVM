// Verifica que readS2, readU2 e readS4 lançam exceção ao ler além do vetor.
// Sem bounds checking, cada chamada abaixo causa comportamento indefinido
// (leitura fora dos limites de heap) em vez de uma exceção limpa.

#include <cassert>
#include <stdexcept>
#include <iostream>
#include <vector>
#include "bytecode_utils.hpp"

static void test_readS2_oob()
{
    std::vector<u1> buf = {0xAB}; // apenas 1 byte; readS2 precisa de 2
    bool threw = false;
    try {
        readS2(buf, 0);
    } catch (const std::runtime_error &) {
        threw = true;
    }
    assert(threw && "readS2: deve lançar runtime_error para acesso fora dos limites");
}

static void test_readU2_oob()
{
    std::vector<u1> buf = {0x00}; // 1 byte; readU2 precisa de 2
    bool threw = false;
    try {
        readU2(buf, 0);
    } catch (const std::runtime_error &) {
        threw = true;
    }
    assert(threw && "readU2: deve lançar runtime_error para acesso fora dos limites");
}

static void test_readS4_oob()
{
    std::vector<u1> buf = {0x00, 0x00, 0x00}; // 3 bytes; readS4 precisa de 4
    bool threw = false;
    try {
        readS4(buf, 0);
    } catch (const std::runtime_error &) {
        threw = true;
    }
    assert(threw && "readS4: deve lançar runtime_error para acesso fora dos limites");
}

static void test_readS2_offset_oob()
{
    // índice válido para o primeiro byte, mas o segundo cai fora
    std::vector<u1> buf = {0x00, 0x01, 0xFF};
    bool threw = false;
    try {
        readS2(buf, 2); // buf[2] existe, buf[3] não
    } catch (const std::runtime_error &) {
        threw = true;
    }
    assert(threw && "readS2: deve lançar quando o segundo byte está fora do vetor");
}

static void test_reads_valid_values()
{
    // Garante que leituras dentro dos limites continuam corretas
    std::vector<u1> buf = {0x00, 0x80, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF};

    assert(readU2(buf, 0) == 0x0080 && "readU2: valor incorreto");
    assert(readS2(buf, 2) == static_cast<int16_t>(0xFFFF) && "readS2: valor incorreto para -1");
    assert(readS4(buf, 4) == static_cast<int32_t>(0x7FFFFFFF) && "readS4: valor incorreto para INT32_MAX");
}

int main()
{
    test_readS2_oob();
    test_readU2_oob();
    test_readS4_oob();
    test_readS2_offset_oob();
    test_reads_valid_values();

    std::cout << "test_bytecode_utils_oob: todos os testes passaram." << std::endl;
    return 0;
}
