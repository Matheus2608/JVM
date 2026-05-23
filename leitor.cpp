#include "leitor.hpp"
#include <iostream>
#include "constants.hpp"
#include <stdexcept>

using std::size_t;

using namespace std;

Leitor::Leitor(const string &filename) : filename(filename), file(filename, ios::binary) {
    if (!file.is_open()) {
        throw std::runtime_error("Leitor: não foi possível abrir o arquivo \"" + filename + "\"");
    }
}

Leitor::~Leitor() = default;

void Leitor::read(u1 *buffer, size_t length)
{
    file.read(reinterpret_cast<char *>(buffer), length);
    if (static_cast<size_t>(file.gcount()) != length) {
        throw std::runtime_error("Leitor: leitura incompleta");
    }
}

std::vector<u1> Leitor::read_bytes(size_t length)
{
    std::vector<u1> buf(length);
    if (length == 0) return buf;
    file.read(reinterpret_cast<char *>(buf.data()), length);
    if (static_cast<size_t>(file.gcount()) != length) {
        throw std::runtime_error("Leitor: leitura incompleta (read_bytes)");
    }
    return buf;
}

u2 Leitor::toBigEndian(u2 value)
{
    return (value >> 8) | (value << 8);
}

u4 Leitor::toBigEndian(u4 value)
{
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8) & 0x0000FF00) |
           ((value << 8) & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

u4 Leitor::readu4()
{
    u1 buffer[4];
    file.read(reinterpret_cast<char *>(buffer), 4);
    if (file.gcount() != 4) throw std::runtime_error("Leitor: leitura incompleta (u4)");
    return (static_cast<u4>(buffer[0]) << 24) |
           (static_cast<u4>(buffer[1]) << 16) |
           (static_cast<u4>(buffer[2]) << 8) |
           static_cast<u4>(buffer[3]);
}

u2 Leitor::readu2()
{
    u1 buffer[2];
    file.read(reinterpret_cast<char *>(buffer), 2);
    if (file.gcount() != 2) throw std::runtime_error("Leitor: leitura incompleta (u2)");
    return (static_cast<u2>(buffer[0]) << 8) |
           static_cast<u2>(buffer[1]);
}

u1 Leitor::readu1()
{
    u1 buffer[1];
    file.read(reinterpret_cast<char *>(buffer), 1);
    if (file.gcount() != 1) throw std::runtime_error("Leitor: leitura incompleta (u1)");
    return buffer[0];
}

size_t Leitor::tell()
{
    auto p = file.tellg();
    if (p == -1) throw std::runtime_error("Leitor: tellg falhou");
    return static_cast<size_t>(p);
}

void Leitor::seek(size_t pos)
{
    file.seekg(static_cast<std::streamoff>(pos));
    if (!file) throw std::runtime_error("Leitor: seekg falhou");
}

void Leitor::require(size_t length)
{
    auto cur = file.tellg();
    if (cur == -1) throw std::runtime_error("Leitor: tellg falhou (require)");
    file.seekg(0, std::ios::end);
    auto end = file.tellg();
    if (end == -1) throw std::runtime_error("Leitor: tellg falhou (require end)");
    file.seekg(cur);
    if (static_cast<size_t>(end - cur) < length) {
        throw std::runtime_error("Leitor: bytes insuficientes no arquivo");
    }
}
