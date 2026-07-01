#ifndef JVM_HEAP_HPP
#define JVM_HEAP_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>

#include "runtime.hpp"          // Value
#include "estrutura_dados.hpp"  // class_info

// -----------------------------------------------------------------------------
// HeapObject — uma instância de classe (resultado de `new`)
//
// Os campos de instância são guardados por nome (string do constant pool),
// cada um com seu Value. O class_info aponta para a classe que foi instanciada,
// usado para resolver hierarquia, getfield/putfield e checagem de tipos.
// -----------------------------------------------------------------------------
struct HeapObject {
    const class_info*                       cls = nullptr; // classe da instância
    std::string                             class_name;    // nome totalmente qualificado
    std::unordered_map<std::string, Value>  fields;        // campos de instância
};

// -----------------------------------------------------------------------------
// HeapArray — um array de uma dimensão (resultado de newarray / anewarray)
//
// Todos os tipos de elemento (int, long, float, double, referência...) são
// guardados como Value. O campo element_type identifica o tipo dos elementos
// para que os opcodes *aload / *astore saibam como interpretá-los.
// -----------------------------------------------------------------------------
struct HeapArray {
    // Códigos de tipo do opcode newarray (JVMS Table 6.5.newarray-A).
    enum ElementType : uint8_t {
        T_BOOLEAN   = 4,
        T_CHAR      = 5,
        T_FLOAT     = 6,
        T_DOUBLE    = 7,
        T_BYTE      = 8,
        T_SHORT     = 9,
        T_INT       = 10,
        T_LONG      = 11,
        T_REFERENCE = 0,  // arrays de objetos (anewarray)
    };

    u1                 element_type = T_REFERENCE;
    std::string        element_class; // nome da classe (apenas para arrays de referência)
    std::vector<Value> elements;
};

// -----------------------------------------------------------------------------
// HeapCell — célula da tabela do heap: ou um objeto, ou um array.
//
// Usa unique_ptr para garantir que apenas uma das duas alternativas existe em
// memória a cada momento — semântica de union sem os problemas de C++11 com
// tipos não-triviais (std::vector / std::unordered_map não cabem em union bruta).
// Em C++17 isso seria std::variant<HeapObject, HeapArray>.
// -----------------------------------------------------------------------------
struct HeapCell {
    enum class Kind { OBJECT, ARRAY } kind = Kind::OBJECT;
    std::unique_ptr<HeapObject> object; // não-nulo quando kind == OBJECT
    std::unique_ptr<HeapArray>  array;  // não-nulo quando kind == ARRAY
};

// =============================================================================
// Heap
// =============================================================================
class Heap {
public:
    Heap() {
        // Slot 0 reservado para representar null — nunca é alocado de verdade.
        cells_.emplace_back();
    }

    // -------------------------------------------------------------------------
    // Alocação
    // -------------------------------------------------------------------------

    /**
     * @brief Aloca um novo objeto no heap.
     * @param cls Ponteiro para a estrutura da classe instanciada.
     * @param class_name Nome da classe.
     * @return A referência (índice no heap) para o objeto alocado.
     */
    int32_t allocateObject(const class_info* cls, const std::string& class_name) {
        HeapCell cell;
        cell.kind          = HeapCell::Kind::OBJECT;
        cell.object.reset(new HeapObject());
        cell.object->cls        = cls;
        cell.object->class_name = class_name;
        cells_.push_back(std::move(cell));
        return static_cast<int32_t>(cells_.size() - 1);
    }

    /**
     * @brief Aloca um novo array de tipo primitivo (resultado de `newarray`).
     * @param element_type O tipo dos elementos, conforme os códigos T_* da JVM.
     * @param length O número de elementos do array.
     * @return A referência para o array alocado.
     * @throws std::runtime_error se o tamanho for negativo.
     */
    int32_t allocateArray(u1 element_type, int32_t length) {
        if (length < 0)
            throw std::runtime_error("NegativeArraySizeException");

        HeapCell cell;
        cell.kind = HeapCell::Kind::ARRAY;
        cell.array.reset(new HeapArray());
        cell.array->element_type = element_type;
        cell.array->elements.assign(static_cast<size_t>(length), defaultFor(element_type));
        cells_.push_back(std::move(cell));
        return static_cast<int32_t>(cells_.size() - 1);
    }

    /**
     * @brief Aloca um novo array de referências (resultado de `anewarray`).
     * @param element_class O nome da classe dos elementos.
     * @param length O número de elementos do array.
     * @return A referência para o array alocado.
     * @throws std::runtime_error se o tamanho for negativo.
     */
    int32_t allocateRefArray(const std::string& element_class, int32_t length) {
        if (length < 0)
            throw std::runtime_error("NegativeArraySizeException");

        HeapCell cell;
        cell.kind = HeapCell::Kind::ARRAY;
        cell.array.reset(new HeapArray());
        cell.array->element_type  = HeapArray::T_REFERENCE;
        cell.array->element_class = element_class;
        cell.array->elements.assign(static_cast<size_t>(length), Value::null());
        cells_.push_back(std::move(cell));
        return static_cast<int32_t>(cells_.size() - 1);
    }

    // -------------------------------------------------------------------------
    // Acesso
    // -------------------------------------------------------------------------

    bool isNull(int32_t ref) const { return ref == 0; }

    HeapObject& object(int32_t ref) {
        HeapCell& cell = at(ref);
        if (cell.kind != HeapCell::Kind::OBJECT)
            throw std::runtime_error("Heap: referencia nao aponta para um objeto");
        return *cell.object;
    }

    HeapArray& array(int32_t ref) {
        HeapCell& cell = at(ref);
        if (cell.kind != HeapCell::Kind::ARRAY)
            throw std::runtime_error("Heap: referencia nao aponta para um array");
        return *cell.array;
    }

    // -------------------------------------------------------------------------
    // Atalhos para getfield / putfield
    // -------------------------------------------------------------------------

    Value getField(int32_t ref, const std::string& name) {
        return object(ref).fields[name];
    }

    void putField(int32_t ref, const std::string& name, Value value) {
        object(ref).fields[name] = value;
    }

    // -------------------------------------------------------------------------
    // Atalhos para *aload / *astore / arraylength
    // -------------------------------------------------------------------------

    int32_t arrayLength(int32_t ref) {
        return static_cast<int32_t>(array(ref).elements.size());
    }

    Value getElement(int32_t ref, int32_t index) {
        HeapArray& a = array(ref);
        checkBounds(a, index);
        return a.elements[static_cast<size_t>(index)];
    }

    void setElement(int32_t ref, int32_t index, Value value) {
        HeapArray& a = array(ref);
        checkBounds(a, index);
        a.elements[static_cast<size_t>(index)] = value;
    }

    size_t size() const { return cells_.size(); }

private:
    std::vector<HeapCell> cells_;

    HeapCell& at(int32_t ref) {
        if (ref == 0)
            throw std::runtime_error("NullPointerException");
        if (ref < 0 || static_cast<size_t>(ref) >= cells_.size())
            throw std::runtime_error("Heap: referencia invalida");
        return cells_[static_cast<size_t>(ref)];
    }

    static void checkBounds(const HeapArray& a, int32_t index) {
        if (index < 0 || static_cast<size_t>(index) >= a.elements.size())
            throw std::runtime_error("ArrayIndexOutOfBoundsException");
    }

    // Valor inicial de cada elemento conforme o tipo (zero/0.0/null).
    static Value defaultFor(u1 element_type) {
        switch (element_type) {
            case HeapArray::T_LONG:      return Value::fromLong(0);
            case HeapArray::T_FLOAT:     return Value::fromFloat(0.0f);
            case HeapArray::T_DOUBLE:    return Value::fromDouble(0.0);
            case HeapArray::T_REFERENCE: return Value::null();
            default:                     return Value::fromInt(0); // boolean/char/byte/short/int
        }
    }
};

#endif // JVM_HEAP_HPP
