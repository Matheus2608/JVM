#ifndef HEAP_HPP
#define HEAP_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

// Forward declaration para Value
struct Value;

// Representa um objeto instanciado (resultado de 'new')
struct JObject {
    std::string class_name;
    std::unordered_map<std::string, Value> fields; // "nomeDoCampo" → valor
};

// Representa um array (resultado de 'newarray' ou 'anewarray')
struct JArray {
    std::string element_type; // "I", "D", "Ljava/lang/String;" etc.
    std::vector<Value> elements;
};

// Classe principal que gerencia a memória do Heap
class Heap {
public:
    Heap();

    // Aloca um objeto e retorna sua referência (um índice > 0)
    int32_t allocObject(const std::string& class_name);

    // Aloca um array e retorna sua referência
    int32_t allocArray(const std::string& type, int32_t size);

    // Acessa um objeto ou array pela sua referência
    JObject& getObject(int32_t ref);
    JArray&  getArray(int32_t ref);

private:
    // Vetores separados para objetos e arrays, conforme especificação 
    std::vector<std::shared_ptr<JObject>> objects_;
    std::vector<std::shared_ptr<JArray>>  arrays_;
};

#endif // HEAP_HPP