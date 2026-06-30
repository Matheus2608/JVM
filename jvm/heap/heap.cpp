#include "heap.hpp"
#include "runtime.hpp" // Para Value e seus construtores
#include <stdexcept>

Heap::Heap() {
    // Reserva o índice 0 em ambos os vetores para representar 'null'.
    // As referências válidas começarão em 1.
    objects_.push_back(nullptr);
    arrays_.push_back(nullptr);
}

int32_t Heap::allocObject(const std::string& class_name) {
    auto new_obj = std::make_shared<JObject>();
    new_obj->class_name = class_name;

    objects_.push_back(new_obj);
    return static_cast<int32_t>(objects_.size() - 1);
}

int32_t Heap::allocArray(const std::string& type, int32_t size) {
    if (size < 0) {
        throw std::runtime_error("NegativeArraySizeException");
    }

    auto new_arr = std::make_shared<JArray>();
    new_arr->element_type = type;

    // Inicializa o array com os valores padrão corretos, conforme a especificação da JVM.
    Value default_value;
    if (type == "J") { // long
        default_value = Value::fromLong(0);
    } else if (type == "D") { // double
        default_value = Value::fromDouble(0.0);
    } else if (type == "F") { // float
        default_value = Value::fromFloat(0.0f);
    } else if (type[0] == 'L' || type[0] == '[') { // Referências (objetos ou outros arrays)
        default_value = Value::null();
    } else { // int, boolean, byte, char, short
        default_value = Value::fromInt(0);
    }

    new_arr->elements.resize(static_cast<size_t>(size), default_value);

    arrays_.push_back(new_arr);
    return static_cast<int32_t>(arrays_.size() - 1);
}

JObject& Heap::getObject(int32_t ref) {
    if (ref == 0) {
        throw std::runtime_error("NullPointerException on object access");
    }
    if (static_cast<size_t>(ref) >= objects_.size()) {
        throw std::runtime_error("Invalid object reference");
    }
    return *objects_[ref];
}

JArray& Heap::getArray(int32_t ref) {
    if (ref == 0) {
        throw std::runtime_error("NullPointerException on array access");
    }
    if (static_cast<size_t>(ref) >= arrays_.size()) {
        throw std::runtime_error("Invalid array reference");
    }
    return *arrays_[ref];
}