#ifndef LEITOR_HPP
#define LEITOR_HPP

#include <fstream>
#include <string>
#include <vector>
#include <cstddef>

#include "estrutura_dados.hpp"

/**
 * @brief Classe responsável pela leitura segura do arquivo binário (.class).
 * 
 * Encapsula a manipulação do stream do arquivo e resolve o problema de "Endianness" 
 * (a JVM utiliza a arquitetura Big-Endian, enquanto processadores modernos usam Little-Endian),
 * fornecendo métodos que leem diretamente os bytes no formato correto.
 */
class Leitor {
public:
    explicit Leitor(const std::string& filename);
    ~Leitor();

    /**
     * @brief Lê um bloco de bytes diretamente para um buffer de memória em ponteiro.
     */
    void read(u1 *buffer, size_t length);

    /**
     * @brief Retorna um vetor dinâmico contendo 'length' bytes avançando o cursor do arquivo.
     */
    std::vector<u1> read_bytes(size_t length);

    // stream position helpers
    size_t tell();
    void seek(size_t pos);

    /**
     * @brief Lê 4 bytes do arquivo e os converte matematicamente para um inteiro de 32 bits (Big-Endian).
     */
    u4 readu4();
    
    /**
     * @brief Lê 2 bytes do arquivo e os converte para um inteiro de 16 bits (Big-Endian).
     */
    u2 readu2();
    
    /**
     * @brief Lê apenas 1 byte do arquivo (u1 / unsigned char).
     */
    u1 readu1();

private:
    std::string filename;
    std::ifstream file;

};

#endif // LEITOR_HPP
