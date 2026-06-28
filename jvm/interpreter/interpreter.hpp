#ifndef JVM_INTERPRETER_HPP
#define JVM_INTERPRETER_HPP

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>
#include "estrutura_dados.hpp"

#include "runtime.hpp"
#include "class_loader.hpp"

class Heap; // implementado depois

// =============================================================================
// Interpreter — Execution Engine
//
// Recebe os três componentes do Runtime Data Area e executa bytecodes.
// O dispatch_table_ mapeia cada opcode para sua função correspondente,
// evitando um switch gigante e permitindo adicionar opcodes incrementalmente.
// =============================================================================

class Interpreter {
public:
    Interpreter(ClassLoader& loader, FrameStack& frame_stack, Heap& heap);

    // Cria o Frame inicial para o método e inicia o loop de execução.
    void execute(const class_info& cls, const method_info& method);

    // Loop principal: busca opcode → despacha via dispatch_table_ → repete.
    void run();

    bool   halted() const { return halted_; }
    void   halt()         { halted_ = true; }

private:
    // -------------------------------------------------------------------------
    // Referências ao Runtime Data Area
    // -------------------------------------------------------------------------
    ClassLoader& loader_;
    FrameStack&  frame_stack_;
    Heap&        heap_;
    bool         halted_ = false;


    // -------------------------------------------------------------------------
    // Dispatch table: opcode → handler
    // Preenchida no construtor uma vez; consultada a cada instrução.
    // -------------------------------------------------------------------------
    std::unordered_map<uint8_t, std::function<void()>> dispatch_table_;

    void buildDispatchTable();

    // -------------------------------------------------------------------------
    // Helpers de leitura de bytecode (avançam o PC do frame atual)
    // -------------------------------------------------------------------------
    Frame& currentFrame();
    u1     fetchU1();
    u2     fetchU2();
    u4     fetchU4();
    int8_t  fetchS1();
    int16_t fetchS2();
    int32_t fetchS4();
    void     branch(int32_t offset); // aplica offset relativo ao PC

    // Localiza o atributo Code de um método
    const code_attribute* findCode(const method_info& method) const;

    // -------------------------------------------------------------------------
    // Constantes
    // -------------------------------------------------------------------------
    void op_nop();
    void op_aconst_null();
    void op_iconst(int32_t value); // helper compartilhado por iconst_m1..5
    void op_iconst_m1();
    void op_iconst_0();
    void op_iconst_1();
    void op_iconst_2();
    void op_iconst_3();
    void op_iconst_4();
    void op_iconst_5();
    void op_lconst_0();
    void op_lconst_1();
    void op_fconst_0();
    void op_fconst_1();
    void op_fconst_2();
    void op_dconst_0();
    void op_dconst_1();
    void op_bipush();
    void op_sipush();
    void op_ldc();
    void op_ldc_w();
    void op_ldc2_w();

    // -------------------------------------------------------------------------
    // Load/Store de variáveis locais
    // -------------------------------------------------------------------------
    void op_iload();   void op_iload_0();  void op_iload_1();  void op_iload_2();  void op_iload_3();
    void op_lload();   void op_lload_0();  void op_lload_1();  void op_lload_2();  void op_lload_3();
    void op_fload();   void op_fload_0();  void op_fload_1();  void op_fload_2();  void op_fload_3();
    void op_dload();   void op_dload_0();  void op_dload_1();  void op_dload_2();  void op_dload_3();
    void op_aload();   void op_aload_0();  void op_aload_1();  void op_aload_2();  void op_aload_3();

    void op_istore();  void op_istore_0(); void op_istore_1(); void op_istore_2(); void op_istore_3();
    void op_lstore();  void op_lstore_0(); void op_lstore_1(); void op_lstore_2(); void op_lstore_3();
    void op_fstore();  void op_fstore_0(); void op_fstore_1(); void op_fstore_2(); void op_fstore_3();
    void op_dstore();  void op_dstore_0(); void op_dstore_1(); void op_dstore_2(); void op_dstore_3();
    void op_astore();  void op_astore_0(); void op_astore_1(); void op_astore_2(); void op_astore_3();

    // -------------------------------------------------------------------------
    // Aritmética inteira
    // -------------------------------------------------------------------------
    void op_iadd(); void op_isub(); void op_imul(); void op_idiv();
    void op_irem(); void op_ineg();
    void op_iand(); void op_ior();  void op_ixor();
    void op_ishl(); void op_ishr(); void op_iushr();
    void op_iinc();

    // -------------------------------------------------------------------------
    // Aritmética long
    // -------------------------------------------------------------------------
    void op_ladd(); void op_lsub(); void op_lmul(); void op_ldiv();
    void op_lrem(); void op_lneg();
    void op_land(); void op_lor();  void op_lxor();
    void op_lshl(); void op_lshr(); void op_lushr();
    void op_lcmp();

    // -------------------------------------------------------------------------
    // Aritmética float / double
    // -------------------------------------------------------------------------
    void op_fadd(); void op_fsub(); void op_fmul(); void op_fdiv(); void op_frem(); void op_fneg();
    void op_dadd(); void op_dsub(); void op_dmul(); void op_ddiv(); void op_drem(); void op_dneg();
    void op_fcmpl(); void op_fcmpg();
    void op_dcmpl(); void op_dcmpg();

    // -------------------------------------------------------------------------
    // Conversões de tipo
    // -------------------------------------------------------------------------
    void op_i2l(); void op_i2f(); void op_i2d();
    void op_l2i(); void op_l2f(); void op_l2d();
    void op_f2i(); void op_f2l(); void op_f2d();
    void op_d2i(); void op_d2l(); void op_d2f();
    void op_i2b(); void op_i2c(); void op_i2s();

    // -------------------------------------------------------------------------
    // Manipulação de pilha
    // -------------------------------------------------------------------------
    void op_pop(); void op_pop2();
    void op_dup(); void op_dup_x1(); void op_dup_x2(); void op_dup2();
    void op_swap();

    // -------------------------------------------------------------------------
    // Controle de fluxo
    // -------------------------------------------------------------------------
    void op_ifeq();       void op_ifne();       void op_iflt();
    void op_ifge();       void op_ifgt();       void op_ifle();
    void op_if_icmpeq();  void op_if_icmpne();  void op_if_icmplt();
    void op_if_icmpge();  void op_if_icmpgt();  void op_if_icmple();
    void op_ifnull();     void op_ifnonnull();
    void op_if_acmpeq();  void op_if_acmpne();
    void op_goto();       void op_goto_w();
    void op_tableswitch(); void op_lookupswitch();

    // -------------------------------------------------------------------------
    // Invocação de métodos
    // -------------------------------------------------------------------------
    void op_invokestatic();
    void op_invokespecial();
    void op_invokevirtual();
    void op_invokeinterface();

    // -------------------------------------------------------------------------
    // Retorno
    // -------------------------------------------------------------------------
    void op_return();
    void op_ireturn(); void op_lreturn(); void op_freturn();
    void op_dreturn(); void op_areturn();

    // -------------------------------------------------------------------------
    // Campos estáticos e de instância
    // -------------------------------------------------------------------------
    void op_getstatic(); void op_putstatic();
    void op_getfield();  void op_putfield();

    // -------------------------------------------------------------------------
    // Objetos e arrays (implementados depois, junto com Heap)
    // -------------------------------------------------------------------------
    void op_new();
    void op_newarray();   void op_anewarray(); void op_arraylength();
    void op_iaload();     void op_iastore();
    void op_laload();     void op_lastore();
    void op_faload();     void op_fastore();
    void op_daload();     void op_dastore();
    void op_aaload();     void op_aastore();
    void op_baload();     void op_bastore();
    void op_caload();     void op_castore();
    void op_saload();     void op_sastore();

    // -------------------------------------------------------------------------
    // Exceções e checagem de tipos
    // -------------------------------------------------------------------------
    void op_athrow();
    void op_checkcast(); void op_instanceof();

    // -------------------------------------------------------------------------
    // Helpers internos
    // -------------------------------------------------------------------------
    // Intercepta chamadas a PrintStream.print/println e imprime direto em stdout.
    void simulatePrint(const std::string& method_name, const std::string& descriptor);
};

#endif // JVM_INTERPRETER_HPP
