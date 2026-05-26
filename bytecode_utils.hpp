#ifndef BYTECODE_UTILS_HPP
#define BYTECODE_UTILS_HPP

#include <vector>
#include <cstdint>
#include <stdexcept>
#include "estrutura_dados.hpp"

/**
 * @brief Utilitários para leitura segura do vetor de bytecodes (Code attribute).
 * 
 * Durante o disassemble, muitas instruções (como goto ou if_icmpeq) possuem offsets 
 * de 2 ou 4 bytes acoplados logo após o opcode. Estas funções garantem a leitura 
 * combinando esses bytes matematicamente (Big-Endian) e prevenindo falhas 
 * de Out of Bounds caso o arquivo esteja mal-formado.
 */

inline int16_t readS2(const std::vector<u1> &c, size_t i)
{
    if (c.size() < 2 || i > c.size() - 2)
        throw std::runtime_error("bytecode: leitura fora dos limites (readS2)");
    return static_cast<int16_t>((static_cast<uint16_t>(c[i]) << 8) | c[i + 1]);
}

inline int32_t readS4(const std::vector<u1> &c, size_t i)
{
    if (c.size() < 4 || i > c.size() - 4)
        throw std::runtime_error("bytecode: leitura fora dos limites (readS4)");
    return static_cast<int32_t>(
        (static_cast<uint32_t>(c[i])     << 24) |
        (static_cast<uint32_t>(c[i + 1]) << 16) |
        (static_cast<uint32_t>(c[i + 2]) <<  8) |
         static_cast<uint32_t>(c[i + 3]));
}

inline uint16_t readU2(const std::vector<u1> &c, size_t i)
{
    if (c.size() < 2 || i > c.size() - 2)
        throw std::runtime_error("bytecode: leitura fora dos limites (readU2)");
    return static_cast<uint16_t>((static_cast<uint16_t>(c[i]) << 8) | c[i + 1]);
}

#endif // BYTECODE_UTILS_HPP
