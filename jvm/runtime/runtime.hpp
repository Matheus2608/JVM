#ifndef JVM_RUNTIME_HPP
#define JVM_RUNTIME_HPP

#include <cstdint>
#include <vector>
#include <stack>
#include <stdexcept>
#include "estrutura_dados.hpp"

// =============================================================================
// Value — tipo universal da pilha de operandos e variáveis locais
// =============================================================================

struct Value {
    enum class Type {
        INT,        // int, boolean, byte, char, short — todos como int32
        LONG,
        FLOAT,
        DOUBLE,
        REFERENCE,  // índice no heap (0 = null)
    } type;

    union {
        int32_t  i;
        int64_t  l;
        float    f;
        double   d;
        int32_t  ref;
    } data;

    static Value fromInt   (int32_t v)  { Value val; val.type = Type::INT;       val.data.i   = v; return val; }
    static Value fromLong  (int64_t v)  { Value val; val.type = Type::LONG;      val.data.l   = v; return val; }
    static Value fromFloat (float v)    { Value val; val.type = Type::FLOAT;     val.data.f   = v; return val; }
    static Value fromDouble(double v)   { Value val; val.type = Type::DOUBLE;    val.data.d   = v; return val; }
    static Value fromRef   (int32_t v)  { Value val; val.type = Type::REFERENCE; val.data.ref = v; return val; }
    static Value null()                 { return fromRef(0); }
};

// =============================================================================
// Frame — estado de execução de uma chamada de método
//
// O PC Register do professor vive aqui dentro (campo pc).
// Criado ao invocar um método, destruído ao retornar.
// =============================================================================

struct Frame {
    std::vector<Value>    locals;
    std::stack<Value>     operand_stack;
    const class_info*     cls;    // para resolver o constant pool
    const method_info*    method; // para exceções e depuração
    const code_attribute* code;   // bytecodes, max_stack, max_locals
    size_t                pc;     // PC Register — índice atual no vetor code->code
    size_t                last_pc = 0; // offset da instrução em execução — usado
                                       // para casar a exception_table no unwinding

    void push(Value v) {
        operand_stack.push(v);
    }

    Value pop() {
        if (operand_stack.empty())
            throw std::runtime_error("Frame: operand stack underflow");
        Value v = operand_stack.top();
        operand_stack.pop();
        return v;
    }

    Value& top() {
        if (operand_stack.empty())
            throw std::runtime_error("Frame: operand stack vazia");
        return operand_stack.top();
    }
};

// =============================================================================
// FrameStack — a Stack do diagrama do professor
//
// Pilha de frames ativos. Cada invocação de método empurra um Frame;
// cada retorno desempurra e restaura o frame do chamador.
// =============================================================================

class FrameStack {
public:
    void push(Frame frame) {
        frames_.push(std::move(frame));
    }

    void pop() {
        if (frames_.empty())
            throw std::runtime_error("FrameStack: pop em pilha vazia");
        frames_.pop();
    }

    Frame& top() {
        if (frames_.empty())
            throw std::runtime_error("FrameStack: top em pilha vazia");
        return frames_.top();
    }

    const Frame& top() const {
        if (frames_.empty())
            throw std::runtime_error("FrameStack: top em pilha vazia");
        return frames_.top();
    }

    bool   empty() const { return frames_.empty(); }
    size_t size()  const { return frames_.size();  }

private:
    std::stack<Frame> frames_;
};

#endif // JVM_RUNTIME_HPP
