# Tarefas — JVM em C++

### Fluxo de execução

```
main.cpp
  └─ ClassLoader::load("MinhaClasse")       → lê e parseia o .class
  └─ findMethod(cls, "main", "([...])V")    → localiza o método
  └─ Interpreter::execute(cls, method)      → cria Frame e inicia o loop
       └─ run(): opcode = fetch() → dispatch_table_[opcode]() → repete
```

### Como cada membro entrega o trabalho

Cada tarefa abaixo corresponde a preencher os **stubs vazios** em `interpreter.cpp`.
Os stubs já existem com o nome certo — basta substituir o `{}` pelo corpo real.

Exemplo de como um stub vira implementação:

```cpp
// ANTES (stub)
void Interpreter::op_ladd() {}

// DEPOIS (implementado)
void Interpreter::op_ladd() {
    Frame& f = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    f.push(Value::fromLong(a + b));
}
```

---

## Tarefa 1 — Load/Store de long, float, double e reference

**Arquivo:** `jvm/interpreter/interpreter.cpp`

**Contexto:** Os opcodes `iload/istore` já estão implementados como modelo.
Seguir o mesmo padrão para os outros tipos.

**Stubs a preencher:**

```
op_lload, op_lload_0..3    → push local como Long
op_fload, op_fload_0..3    → push local como Float
op_dload, op_dload_0..3    → push local como Double
op_aload, op_aload_0..3    → push local como Reference

op_lstore, op_lstore_0..3  → pop Long e guarda no local
op_fstore, op_fstore_0..3  → pop Float e guarda no local
op_dstore, op_dstore_0..3  → pop Double e guarda no local
op_astore, op_astore_0..3  → pop Reference e guarda no local
```

**Modelo (copiar padrão do iload/istore já implementado):**
```cpp
void Interpreter::op_lload_0() {
    Frame& f = currentFrame();
    f.push(f.locals[0]);   // o Value já carrega o tipo dentro dele
}

void Interpreter::op_lload() {
    Frame& f    = currentFrame();
    uint8_t idx = fetchU1(); // lê o índice do local do próximo byte
    f.push(f.locals[idx]);
}

void Interpreter::op_lstore_0() {
    Frame& f = currentFrame();
    f.locals[0] = f.pop();
}
```

**Teste:** `exemplos/double_aritmetica.class`

---

## Tarefa 2 — ldc, ldc_w, ldc2_w (carregar constantes do Constant Pool)

**Arquivo:** `jvm/interpreter/interpreter.cpp`

**Contexto:** Esses opcodes lêem um índice do bytecode e empurram o valor
correspondente da Constant Pool do arquivo .class na pilha.
A função `cpEntryComment` em `cp_utils.hpp` já resolve entradas da CP —
mas para execução você precisa do **valor**, não da string formatada.

**Stubs a preencher:** `op_ldc`, `op_ldc_w`, `op_ldc2_w`

**Como funciona:**

```cpp
void Interpreter::op_ldc() {
    Frame& f    = currentFrame();
    uint8_t idx = fetchU1();                        // índice de 1 byte
    const cp_info& entry = f.cls->constant_pool[idx];

    if (entry.tag == CONSTANT_Integer)
        f.push(Value::fromInt(static_cast<int32_t>(entry.container.Integer.bytes)));
    else if (entry.tag == CONSTANT_Float) {
        float val;
        uint32_t bits = entry.container.Float.bytes;
        memcpy(&val, &bits, sizeof(float));
        f.push(Value::fromFloat(val));
    }
    else if (entry.tag == CONSTANT_String)
        f.push(Value::fromRef(0)); // stub: strings são objetos — implementar junto com Heap
}

void Interpreter::op_ldc_w() {
    // igual ao ldc mas o índice tem 2 bytes (fetchU2)
}

void Interpreter::op_ldc2_w() {
    // para Long e Double (índice de 2 bytes, valor de 8 bytes)
    Frame& f     = currentFrame();
    uint16_t idx = fetchU2();
    const cp_info& entry = f.cls->constant_pool[idx];

    if (entry.tag == CONSTANT_Long) {
        int64_t val = (static_cast<int64_t>(entry.container.Long.high_bytes) << 32)
                    |  entry.container.Long.low_bytes;
        f.push(Value::fromLong(val));
    } else if (entry.tag == CONSTANT_Double) {
        uint64_t bits = (static_cast<uint64_t>(entry.container.Double.high_bytes) << 32)
                      |  entry.container.Double.low_bytes;
        double val;
        memcpy(&val, &bits, sizeof(double));
        f.push(Value::fromDouble(val));
    }
}
```

**Teste:** qualquer `.class` com literais numéricos ou strings

---

## Tarefa 3 — Aritmética long, float e double

**Arquivo:** `jvm/interpreter/interpreter.cpp`

**Contexto:** Os opcodes inteiros (`iadd`, `isub`, etc.) já estão implementados como modelo.
Seguir o mesmo padrão para long, float e double.

**Stubs a preencher:**

```
ladd, lsub, lmul, ldiv, lrem, lneg
land, lor, lxor, lshl, lshr, lushr
lcmp

fadd, fsub, fmul, fdiv, frem, fneg
fcmpl, fcmpg

dadd, dsub, dmul, ddiv, drem, dneg
dcmpl, dcmpg
```

**Atenção ao lcmp, fcmpl/g, dcmpl/g:** retornam -1, 0 ou 1 na pilha.

```cpp
void Interpreter::op_lcmp() {
    Frame& f  = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    f.push(Value::fromInt(a > b ? 1 : (a < b ? -1 : 0)));
}

// fcmpg: NaN → push 1; fcmpl: NaN → push -1
void Interpreter::op_fcmpg() {
    Frame& f = currentFrame();
    float b  = f.pop().data.f;
    float a  = f.pop().data.f;
    if (std::isnan(a) || std::isnan(b)) { f.push(Value::fromInt(1));  return; }
    f.push(Value::fromInt(a > b ? 1 : (a < b ? -1 : 0)));
}
```

**Teste:** `exemplos/double_aritmetica.class`, `exemplos/double_cast.class`

---

## Tarefa 4 — Conversões de tipo e manipulação de pilha

**Arquivo:** `jvm/interpreter/interpreter.cpp`

**Stubs a preencher:**

Conversões:
```
i2l, i2f, i2d
l2i, l2f, l2d
f2i, f2l, f2d
d2i, d2l, d2f
i2b, i2c, i2s
```

Pilha:
```
pop, pop2
dup, dup_x1, dup_x2, dup2
swap
```

**Exemplos:**
```cpp
void Interpreter::op_i2l() {
    Frame& f = currentFrame();
    int32_t v = f.pop().data.i;
    f.push(Value::fromLong(static_cast<int64_t>(v)));
}

void Interpreter::op_i2b() {
    Frame& f = currentFrame();
    int32_t v = f.pop().data.i;
    f.push(Value::fromInt(static_cast<int8_t>(v))); // trunca para 8 bits com sinal
}

void Interpreter::op_dup() {
    Frame& f = currentFrame();
    f.push(f.top()); // duplica o topo sem removê-lo
}

void Interpreter::op_pop() {
    currentFrame().pop();
}

void Interpreter::op_swap() {
    Frame& f  = currentFrame();
    Value top = f.pop();
    Value sec = f.pop();
    f.push(top);
    f.push(sec);
}
```

**Teste:** `exemplos/double_cast.class`

---

## Tarefa 5 — Controle de fluxo (branches e switches)

**Arquivo:** `jvm/interpreter/interpreter.cpp`

**Stubs a preencher:**

```
ifeq, ifne, iflt, ifge, ifgt, ifle
if_icmpeq, if_icmpne, if_icmplt, if_icmpge, if_icmpgt, if_icmple
ifnull, ifnonnull
if_acmpeq, if_acmpne
goto_w
tableswitch, lookupswitch
```

**Como funciona o branch:**
O offset é lido como `int16_t` (2 bytes, com sinal). A função `branch(offset)` já existe
e ajusta o PC. Para `goto_w` o offset é `int32_t` (4 bytes).

```cpp
void Interpreter::op_ifeq() {
    int16_t offset = fetchS2();
    Frame& f = currentFrame();
    int32_t val = f.pop().data.i;
    if (val == 0)
        f.pc += offset - 3; // -3 = desfaz 1 byte do opcode + 2 bytes do operando
}

void Interpreter::op_if_icmplt() {
    int16_t offset = fetchS2();
    Frame& f  = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    if (a < b)
        f.pc += offset - 3;
}
```

**tableswitch** e **lookupswitch** já têm lógica no `disasm.cpp` — usar como referência.

**Atenção:** o PC deve ser alinhado a 4 bytes antes de ler os operandos do switch:
```cpp
void Interpreter::op_tableswitch() {
    Frame& f = currentFrame();
    // alinha para múltiplo de 4 (pc já avançou 1 para o opcode)
    while (f.pc % 4 != 0) f.pc++;
    int32_t def    = static_cast<int32_t>(fetchU4());
    int32_t low    = static_cast<int32_t>(fetchU4());
    int32_t high   = static_cast<int32_t>(fetchU4());
    // lê (high - low + 1) offsets
    // ...
}
```

**Teste:** `exemplos/tableswitch.class`, `exemplos/lookupswitch.class`, `exemplos/fatorial.class`

---

## Tarefa 6 — Retorno de métodos e operadores lógicos inteiros restantes

**Arquivo:** `jvm/interpreter/interpreter.cpp`

**Stubs a preencher:**

```
lreturn, freturn, dreturn, areturn
iand, ior, ixor, ishl, ishr, iushr
```

**Modelo (seguir op_ireturn já implementado):**
```cpp
void Interpreter::op_lreturn() {
    Value retval = frame_stack_.top().pop();
    frame_stack_.pop();
    if (!frame_stack_.empty())
        frame_stack_.top().push(retval);
}

void Interpreter::op_iand() {
    Frame& f  = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    f.push(Value::fromInt(a & b));
}

void Interpreter::op_ishl() {
    Frame& f  = currentFrame();
    int32_t b = f.pop().data.i & 0x1f; // só os 5 bits menos significativos
    int32_t a = f.pop().data.i;
    f.push(Value::fromInt(a << b));
}
```

---

## Tarefa 7 — Invocação de métodos

**Arquivo:** `jvm/interpreter/interpreter.cpp`

**Stubs a preencher:** `op_invokestatic`, `op_invokespecial`, `op_invokevirtual`, `op_invokeinterface`

**Contexto:** Esta é a tarefa mais complexa. Requer:
1. Ler o índice do Constant Pool do bytecode (`fetchU2`)
2. Resolver o nome da classe, método e descritor a partir da CP
3. Chamar `loader_.load(class_name)` para garantir que a classe está carregada
4. Chamar `loader_.markInitialized` / executar `<clinit>` se necessário
5. Criar um novo `Frame` com os argumentos transferidos da pilha atual para os `locals` do novo frame
6. Empurrar o frame na `frame_stack_` — o `run()` continua automaticamente no novo frame

**Esqueleto:**
```cpp
void Interpreter::op_invokestatic() {
    uint16_t idx = fetchU2();
    Frame& caller = currentFrame();
    const cp_info& entry = caller.cls->constant_pool[idx]; // CONSTANT_Methodref

    std::string class_name  = classNameFromConstantPool(*caller.cls, entry.container.Methodref.class_index);
    std::string method_name = /* resolver via NameAndType */;
    std::string descriptor  = /* resolver via NameAndType */;

    const class_info& target_cls = loader_.load(class_name);

    // executar <clinit> na primeira vez
    if (!loader_.isInitialized(class_name)) {
        loader_.markInitialized(class_name);
        const method_info* clinit = findMethod(target_cls, "<clinit>", "()V");
        if (clinit) execute(target_cls, *clinit);
    }

    const method_info* method = findMethod(target_cls, method_name, descriptor);
    if (!method)
        throw std::runtime_error("Metodo nao encontrado: " + method_name);

    const code_attribute* code = findCode(*method);

    // transferir argumentos da pilha do caller para os locals do novo frame
    // (parsear o descritor para saber quantos argumentos)

    Frame new_frame;
    new_frame.cls    = &target_cls;
    new_frame.method = method;
    new_frame.code   = code;
    new_frame.pc     = 0;
    new_frame.locals.resize(code->max_locals, Value::fromInt(0));
    // popular new_frame.locals com os args retirados de caller.operand_stack

    frame_stack_.push(std::move(new_frame));
    // run() continua no novo frame automaticamente
}
```

**Diferença entre os tipos de invoke:**
- `invokestatic` — sem `this`, args começam em `locals[0]`
- `invokespecial` — construtores e `super.*`; `this` vai em `locals[0]`
- `invokevirtual` — despacho pelo tipo real do objeto; `this` em `locals[0]`
- `invokeinterface` — similar ao virtual, tem um byte extra de contagem de args no bytecode

**Ferramentas disponíveis:** `classNameFromConstantPool`, `utf8FromConstantPool`, `nameAndTypeStr` — todas em `cp_utils.hpp`.

**Teste:** `exemplos/soma_certo.class`, `exemplos/method_test.class`

---

## Tarefa 8 — Campos estáticos e de instância

**Arquivo:** `jvm/interpreter/interpreter.cpp`

**Stubs a preencher:** `op_getstatic`, `op_putstatic`, `op_getfield`, `op_putfield`

**Contexto:**
- `getstatic` / `putstatic` usam `loader_.getStaticField` / `loader_.setStaticField`
- `getfield` / `putfield` operam em `JObject` no heap — dependem da Tarefa 9

**Caso especial obrigatório — `System.out.println`:**
`getstatic` sobre `java/lang/System.out` e `invokevirtual` sobre `java/io/PrintStream.println`
não podem ser executados (não temos a biblioteca Java). Detectar e redirecionar para `std::cout`:

```cpp
void Interpreter::op_getstatic() {
    uint16_t idx = fetchU2();
    // resolver class_name e field_name via CP...

    if (class_name == "java/lang/System" && field_name == "out") {
        currentFrame().push(Value::fromRef(-1)); // ref especial para System.out
        return;
    }

    std::string key = class_name + "::" + field_name;
    currentFrame().push(Value::fromInt(loader_.getStaticField(key)));
}
```

```cpp
// Em op_invokevirtual, antes de resolver normalmente:
if (class_name == "java/io/PrintStream" && method_name == "println") {
    Value arg = currentFrame().pop();
    currentFrame().pop(); // remove a ref do PrintStream
    // imprimir conforme o tipo do arg
    std::cout << arg.data.i << std::endl;
    return;
}
```

**Teste:** `exemplos/Hello.class`

---

## Tarefa 9 — Heap: objetos e arrays

**Arquivo:** `jvm/heap/heap.hpp` (criar `.cpp` se necessário)

**Contexto:** O heap é um vetor de objetos alocados. Uma `reference` em `Value`
é um índice inteiro nesse vetor. `ref = 0` significa `null`.

**Estruturas a definir em `heap.hpp`:**

```cpp
struct JObject {
    std::string class_name;
    std::unordered_map<std::string, Value> fields; // "nomeDoCampo" → valor
};

struct JArray {
    std::string element_type; // "I", "D", "Ljava/lang/String;" etc.
    std::vector<Value> elements;
};

class Heap {
public:
    int32_t allocObject(const std::string& class_name);  // retorna ref
    int32_t allocArray(const std::string& type, int32_t size); // retorna ref

    JObject& getObject(int32_t ref);
    JArray&  getArray(int32_t ref);

private:
    std::vector<std::shared_ptr<JObject>> objects_; // índice 0 = null (sempre vazio)
    std::vector<std::shared_ptr<JArray>>  arrays_;
};
```

**Stubs a preencher em `interpreter.cpp`:**

```
op_new          → Heap::allocObject, empurra ref
op_newarray     → Heap::allocArray para tipos primitivos
op_anewarray    → Heap::allocArray para referências
op_arraylength  → Heap::getArray(ref).elements.size()
op_iaload/iastore, op_laload/lastore, ... → Heap::getArray
op_aaload/aastore → Heap::getArray
```

**Teste:** `exemplos/vetor2.class`, `exemplos/vetor_8.class`

---

## Tarefa 10 — Exceções

**Arquivo:** `jvm/interpreter/interpreter.cpp`

**Stub a preencher:** `op_athrow`, `op_checkcast`, `op_instanceof`

**Como funciona `athrow`:**
1. Pop da pilha → referência do objeto de exceção
2. Percorre a `exception_table` do frame atual buscando um handler compatível
3. Se encontrar: ajusta o PC para `handler_pc` e empurra a referência de volta na pilha
4. Se não encontrar: desempilha o frame e repete no frame anterior (propagação)
5. Se a pilha esvaziar sem handler: imprime a exceção e encerra

```cpp
void Interpreter::op_athrow() {
    Value exc_ref = currentFrame().pop();

    while (!frame_stack_.empty()) {
        Frame& f = currentFrame();
        for (const exception_table_info& entry : f.code->exception_table) {
            if (f.pc > entry.start_pc && f.pc <= entry.end_pc) {
                // entry.catch_type == 0 → captura qualquer exceção (finally)
                f.pc = entry.handler_pc;
                f.push(exc_ref);
                return;
            }
        }
        frame_stack_.pop();
    }
    throw std::runtime_error("Exception nao capturada");
}
```

**Teste:** criar um `.java` simples com try/catch e compilar

---

## Resumo de dependências

```
Tarefa 1 (load/store)     → nenhuma
Tarefa 2 (ldc)            → nenhuma
Tarefa 3 (aritmética)     → nenhuma
Tarefa 4 (conversões)     → nenhuma
Tarefa 5 (branches)       → Tarefas 1-4 para ter algo para testar
Tarefa 6 (retorno/lógico) → nenhuma
Tarefa 7 (invoke)         → Tarefas 1-6 + ClassLoader (já pronto)
Tarefa 8 (campos)         → Tarefa 7 + Heap parcial
Tarefa 9 (heap/arrays)    → Tarefa 7
Tarefa 10 (exceções)      → Tarefas 7-9
```

As tarefas 1 a 6 são **independentes entre si** e podem ser feitas em paralelo.
A tarefa 7 desbloqueia todas as demais.
