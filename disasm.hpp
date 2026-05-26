#ifndef DISASM_HPP
#define DISASM_HPP

#include <vector>
#include "estrutura_dados.hpp"

/**
 * @brief Motor de decodificação de Bytecodes (Disassembler).
 * 
 * Traduz o array de bytes bruto do atributo 'Code' em instruções assembly legíveis 
 * da JVM (mnemônicos como aload_0, invokespecial). Resolve offsets de pulo (branching)
 * e lida com instruções avançadas de tamanho variável (como tableswitch e lookupswitch).
 */
void disassembleCode(const std::vector<u1> &code, u4 code_length, const class_info &cls);

#endif // DISASM_HPP
