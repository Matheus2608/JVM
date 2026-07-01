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
#include "disasm.hpp"
#include "estrutura_dados.hpp"

// Includes para portabilidade de pipes (Windows/Linux)
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

static class_info empty_cls;

// Captura stdout durante a execução de disassembleCode.
static std::string capture_disasm(const std::vector<u1> &code, u4 len)
{
    // Redireciona stdout para um pipe temporário
    fflush(stdout);

#ifdef _WIN32
    int original_stdout_fd = _dup(STDOUT_FILENO);
    int pipefd[2];
    // O_BINARY é importante para evitar conversão de \n para \r\n
    if (_pipe(pipefd, 1024, _O_BINARY) != 0) {
        perror("Falha ao criar pipe");
        return "";
    }
    _dup2(pipefd[1], STDOUT_FILENO);
    _close(pipefd[1]);
#else
    int saved_stdout = dup(STDOUT_FILENO);
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        perror("Falha ao criar pipe");
        return "";
    }
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
#endif

    disassembleCode(code, len, empty_cls);
    fflush(stdout);

#ifdef _WIN32
    _dup2(original_stdout_fd, STDOUT_FILENO);
    _close(original_stdout_fd);
#else
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
#endif

    char buf[1024] = {};
    // MinGW define read/close em io.h, então isso funciona em ambos os sistemas
    int n = read(pipefd[0], buf, sizeof(buf) - 1);
    close(pipefd[0]);
    if (n > 0) buf[n] = '\0';
    return std::string(buf);
}

static void test_goto_target_positivo()
{
    // Instrução: goto com offset +5 (0xA7 00 05).
    // A partir do pc=0, o alvo do desvio deve ser 0 + 5 = 5.
    std::vector<u1> code = {0xA7, 0x00, 0x05};
    std::string output = capture_disasm(code, 3);
    assert(output.find("goto 5") != std::string::npos
           && "goto com offset +5 deve produzir target 5");
}

static void test_goto_target_negativo()
{
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
