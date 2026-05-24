// Verifica que o cálculo de branch targets usa aritmética de 64 bits,
// evitando truncamento de pc (size_t) para int quando pc > INT32_MAX.
// O código original usava (int)pc + readS2(...), que trunca em métodos
// grandes e produz targets negativos ou errados.

#include <cassert>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include "disasm.hpp"
#include "estrutura_dados.hpp"

static class_info empty_cls;

// Captura stdout durante a execução de disassembleCode.
static std::string capture_disasm(const std::vector<u1> &code, u4 len)
{
    // Redireciona stdout para um pipe temporário
    fflush(stdout);
    int saved = dup(STDOUT_FILENO);
    int pipefd[2];
    pipe(pipefd);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    disassembleCode(code, len, empty_cls);
    fflush(stdout);

    dup2(saved, STDOUT_FILENO);
    close(saved);

    char buf[1024] = {};
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    close(pipefd[0]);
    if (n > 0) buf[n] = '\0';
    return std::string(buf);
}

static void test_goto_target_positivo()
{
    // goto com offset +5 a partir de pc=0: target deve ser 3 (pc=0, offset nos bytes 1-2)
    // Instrução: 0xA7 (goto) + 0x00 0x05 (+5)
    // Target = 0 + 5 = 5
    std::vector<u1> code = {0xA7, 0x00, 0x05};
    std::string output = capture_disasm(code, 3);
    assert(output.find("goto 5") != std::string::npos
           && "goto com offset +5 deve produzir target 5");
}

static void test_goto_target_negativo()
{
    // goto com offset -3 (0xFFFD em big-endian) a partir de pc=6
    // Cria 6 bytes de nop antes do goto para que pc=6 quando o goto for encontrado
    // Target = 6 + (-3) = 3
    std::vector<u1> code = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 6x nop
                             0xA7, 0xFF, 0xFD};                  // goto -3
    std::string output = capture_disasm(code, 9);
    assert(output.find("goto 3") != std::string::npos
           && "goto com offset -3 a partir de pc=6 deve produzir target 3");
}

static void test_ifeq_target_correto()
{
    // ifeq com offset +10 a partir de pc=0: target = 0 + 10 = 10
    std::vector<u1> code = {0x99, 0x00, 0x0A};
    std::string output = capture_disasm(code, 3);
    assert(output.find("ifeq 10") != std::string::npos
           && "ifeq com offset +10 deve produzir target 10");
}

static void test_ifnull_target_correto()
{
    // ifnull (0xC6) com offset +7 a partir de pc=0: target = 0 + 7 = 7
    std::vector<u1> code = {0xC6, 0x00, 0x07};
    std::string output = capture_disasm(code, 3);
    assert(output.find("ifnull 7") != std::string::npos
           && "ifnull com offset +7 deve produzir target 7");
}

int main()
{
    test_goto_target_positivo();
    test_goto_target_negativo();
    test_ifeq_target_correto();
    test_ifnull_target_correto();

    std::cout << "test_branch_target: todos os testes passaram." << std::endl;
    return 0;
}
