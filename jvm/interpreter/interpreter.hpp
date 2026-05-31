#ifndef JVM_INTERPRETER_HPP
#define JVM_INTERPRETER_HPP

#include <cstddef>
#include <cstdint>
#include <string>

struct class_info;
struct method_info;

struct Frame;
struct Value;

class MethodArea;
class FrameStack;
class Heap;

class Interpreter {
public:
    Interpreter(MethodArea &method_area, FrameStack &frame_stack, Heap &heap);

    void execute(class_info &cls, method_info &method);
    void step();
    void run();

    std::size_t programCounter() const;
    const Frame &currentFrame() const;

    bool isHalted() const;
    void halt();

private:
    // Core fetch/decode helpers
    std::uint8_t fetchU1();
    std::uint16_t fetchU2();
    std::uint32_t fetchU4();
    std::int8_t fetchS1();
    std::int16_t fetchS2();
    std::int32_t fetchS4();
    void dispatch(std::uint8_t opcode);
    void branch(std::int32_t offset);

    // Constants
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
    void op_aconst_null();
    void op_bipush();
    void op_sipush();
    void op_ldc();
    void op_ldc_w();
    void op_ldc2_w();

    // Load/Store (locals)
    void op_iload();
    void op_iload_0();
    void op_iload_1();
    void op_iload_2();
    void op_iload_3();
    void op_istore();
    void op_istore_0();
    void op_istore_1();
    void op_istore_2();
    void op_istore_3();

    void op_lload();
    void op_lload_0();
    void op_lload_1();
    void op_lload_2();
    void op_lload_3();
    void op_lstore();
    void op_lstore_0();
    void op_lstore_1();
    void op_lstore_2();
    void op_lstore_3();

    void op_fload();
    void op_fload_0();
    void op_fload_1();
    void op_fload_2();
    void op_fload_3();
    void op_fstore();
    void op_fstore_0();
    void op_fstore_1();
    void op_fstore_2();
    void op_fstore_3();

    void op_dload();
    void op_dload_0();
    void op_dload_1();
    void op_dload_2();
    void op_dload_3();
    void op_dstore();
    void op_dstore_0();
    void op_dstore_1();
    void op_dstore_2();
    void op_dstore_3();

    void op_aload();
    void op_aload_0();
    void op_aload_1();
    void op_aload_2();
    void op_aload_3();
    void op_astore();
    void op_astore_0();
    void op_astore_1();
    void op_astore_2();
    void op_astore_3();

    // Integer arithmetic
    void op_iadd();
    void op_isub();
    void op_imul();
    void op_idiv();
    void op_irem();
    void op_ineg();
    void op_iand();
    void op_ior();
    void op_ixor();
    void op_ishl();
    void op_ishr();
    void op_iushr();
    void op_iinc();

    // Long arithmetic
    void op_ladd();
    void op_lsub();
    void op_lmul();
    void op_ldiv();
    void op_lrem();
    void op_lneg();
    void op_land();
    void op_lor();
    void op_lxor();
    void op_lshl();
    void op_lshr();
    void op_lushr();

    // Float/Double arithmetic
    void op_fadd();
    void op_fsub();
    void op_fmul();
    void op_fdiv();
    void op_frem();
    void op_fneg();
    void op_dadd();
    void op_dsub();
    void op_dmul();
    void op_ddiv();
    void op_drem();
    void op_dneg();

    // Stack manipulation
    void op_pop();
    void op_pop2();
    void op_dup();
    void op_dup_x1();
    void op_dup_x2();
    void op_dup2();
    void op_swap();

    // Type conversions
    void op_i2l();
    void op_i2f();
    void op_i2d();
    void op_l2i();
    void op_l2f();
    void op_l2d();
    void op_f2i();
    void op_f2l();
    void op_f2d();
    void op_d2i();
    void op_d2l();
    void op_d2f();
    void op_i2b();
    void op_i2c();
    void op_i2s();

    // Comparisons
    void op_lcmp();
    void op_fcmpl();
    void op_fcmpg();
    void op_dcmpl();
    void op_dcmpg();

    // Conditional branches
    void op_ifeq();
    void op_ifne();
    void op_iflt();
    void op_ifge();
    void op_ifgt();
    void op_ifle();
    void op_if_icmpeq();
    void op_if_icmpne();
    void op_if_icmplt();
    void op_if_icmpge();
    void op_if_icmpgt();
    void op_if_icmple();
    void op_ifnull();
    void op_ifnonnull();
    void op_if_acmpeq();
    void op_if_acmpne();

    // Jumps / control flow
    void op_goto();
    void op_goto_w();
    void op_jsr();
    void op_ret();
    void op_tableswitch();
    void op_lookupswitch();

    // Method invocation
    void op_invokestatic();
    void op_invokespecial();
    void op_invokevirtual();
    void op_invokeinterface();

    // Returns
    void op_ireturn();
    void op_lreturn();
    void op_freturn();
    void op_dreturn();
    void op_areturn();
    void op_return();

    // Field access
    void op_getstatic();
    void op_putstatic();
    void op_getfield();
    void op_putfield();

    // Objects and arrays
    void op_new();
    void op_newarray();
    void op_anewarray();
    void op_arraylength();
    void op_iaload();
    void op_iastore();
    void op_aaload();
    void op_aastore();
    void op_baload();
    void op_bastore();
    void op_caload();
    void op_castore();
    void op_saload();
    void op_sastore();
    void op_laload();
    void op_lastore();
    void op_faload();
    void op_fastore();
    void op_daload();
    void op_dastore();

    // Exceptions / type checks
    void op_athrow();
    void op_checkcast();
    void op_instanceof();

    MethodArea &methodArea;
    FrameStack &frameStack;
    Heap &heap;

    bool halted;
};

#endif // JVM_INTERPRETER_HPP
