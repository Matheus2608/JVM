#include "interpreter.hpp"
#include "cp_utils.hpp"
#include "heap.hpp"

#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>

namespace {

/**
 * @brief Representa uma exceção Java em trânsito no interpretador C++.
 * @param ref A referência do objeto da exceção alocado no heap.
 * @param class_name O nome da classe da exceção, usado para encontrar o handler `catch` correto.
 */
struct JavaException {
    int32_t     ref;
    std::string class_name;
};

const std::unordered_map<std::string, std::string>& builtinThrowableParents() {
    static const std::unordered_map<std::string, std::string> m = {
        {"java/lang/ArithmeticException",             "java/lang/RuntimeException"},
        {"java/lang/NullPointerException",            "java/lang/RuntimeException"},
        {"java/lang/ClassCastException",              "java/lang/RuntimeException"},
        {"java/lang/NegativeArraySizeException",      "java/lang/RuntimeException"},
        {"java/lang/ArrayStoreException",             "java/lang/RuntimeException"},
        {"java/lang/IllegalArgumentException",        "java/lang/RuntimeException"},
        {"java/lang/IllegalStateException",           "java/lang/RuntimeException"},
        {"java/lang/ArrayIndexOutOfBoundsException",  "java/lang/IndexOutOfBoundsException"},
        {"java/lang/StringIndexOutOfBoundsException", "java/lang/IndexOutOfBoundsException"},
        {"java/lang/IndexOutOfBoundsException",       "java/lang/RuntimeException"},
        {"java/lang/RuntimeException",                "java/lang/Exception"},
        {"java/lang/Exception",                       "java/lang/Throwable"},
        {"java/lang/Error",                           "java/lang/Throwable"},
        {"java/lang/Throwable",                       "java/lang/Object"},
    };
    return m;
}

bool throwableIsSubtype(ClassLoader& loader, std::string cur, const std::string& target) {
    if (target == "java/lang/Object" || target == "java/lang/Throwable")
        return true;
    // Limita a busca para evitar loops infinitos em hierarquias malformadas.
    for (int guard = 0; guard < 64; ++guard) {
        if (cur == target)
            return true;
        std::string next;
        try {
            const class_info& cls = loader.load(cur);
            if (cls.super_class == 0)
                return false;
            next = classNameFromConstantPool(cls, cls.super_class);
        } catch (...) {
            auto it = builtinThrowableParents().find(cur);
            if (it == builtinThrowableParents().end())
                return false;
            next = it->second;
        }
        cur = next;
    }
    return false;
}

bool dispatchException(FrameStack& fs, ClassLoader& loader,
                       int32_t ref, const std::string& ex_class) {
    while (!fs.empty()) {
        Frame& f = fs.top();
        size_t pc = f.last_pc;

        for (const exception_table_info& ent : f.code->exception_table) {
            if (pc < ent.start_pc || pc >= ent.end_pc)
                continue;

            bool matches = (ent.catch_type == 0);
            if (!matches) {
                std::string catch_class = classNameFromConstantPool(*f.cls, ent.catch_type);
                matches = throwableIsSubtype(loader, ex_class, catch_class);
            }
            if (matches) {
                while (!f.operand_stack.empty()) f.operand_stack.pop();
                f.push(Value::fromRef(ref));
                f.pc = ent.handler_pc;
                return true;
            }
        }
        fs.pop();
    }
    return false;
}

std::string builtinClassForMessage(const std::string& msg) {
    static const std::pair<const char*, const char*> table[] = {
        {"ArithmeticException",            "java/lang/ArithmeticException"},
        {"NullPointerException",           "java/lang/NullPointerException"},
        {"ArrayIndexOutOfBoundsException", "java/lang/ArrayIndexOutOfBoundsException"},
        {"NegativeArraySizeException",     "java/lang/NegativeArraySizeException"},
        {"ClassCastException",             "java/lang/ClassCastException"},
    };
    for (const auto& m : table)
        if (msg.find(m.first) != std::string::npos)
            return m.second;
    return "";
}

} // namespace

Interpreter::Interpreter(ClassLoader& loader, FrameStack& frame_stack, Heap& heap)
    : loader_(loader), frame_stack_(frame_stack), heap_(heap)
{
    buildDispatchTable();
}

void Interpreter::buildDispatchTable() {
    dispatch_table_[0x00] = [this]{ op_nop();         };
    dispatch_table_[0x01] = [this]{ op_aconst_null(); };
    dispatch_table_[0x02] = [this]{ op_iconst_m1();   };
    dispatch_table_[0x03] = [this]{ op_iconst_0();    };
    dispatch_table_[0x04] = [this]{ op_iconst_1();    };
    dispatch_table_[0x05] = [this]{ op_iconst_2();    };
    dispatch_table_[0x06] = [this]{ op_iconst_3();    };
    dispatch_table_[0x07] = [this]{ op_iconst_4();    };
    dispatch_table_[0x08] = [this]{ op_iconst_5();    };
    dispatch_table_[0x09] = [this]{ op_lconst_0();    };
    dispatch_table_[0x0a] = [this]{ op_lconst_1();    };
    dispatch_table_[0x0b] = [this]{ op_fconst_0();    };
    dispatch_table_[0x0c] = [this]{ op_fconst_1();    };
    dispatch_table_[0x0d] = [this]{ op_fconst_2();    };
    dispatch_table_[0x0e] = [this]{ op_dconst_0();    };
    dispatch_table_[0x0f] = [this]{ op_dconst_1();    };
    dispatch_table_[0x10] = [this]{ op_bipush();      };
    dispatch_table_[0x11] = [this]{ op_sipush();      };
    dispatch_table_[0x12] = [this]{ op_ldc();         };
    dispatch_table_[0x13] = [this]{ op_ldc_w();       };
    dispatch_table_[0x14] = [this]{ op_ldc2_w();      };

    dispatch_table_[0x15] = [this]{ op_iload();   }; dispatch_table_[0x1a] = [this]{ op_iload_0(); };
    dispatch_table_[0x1b] = [this]{ op_iload_1(); }; dispatch_table_[0x1c] = [this]{ op_iload_2(); };
    dispatch_table_[0x1d] = [this]{ op_iload_3(); };
    dispatch_table_[0x16] = [this]{ op_lload();   }; dispatch_table_[0x1e] = [this]{ op_lload_0(); };
    dispatch_table_[0x1f] = [this]{ op_lload_1(); }; dispatch_table_[0x20] = [this]{ op_lload_2(); };
    dispatch_table_[0x21] = [this]{ op_lload_3(); };
    dispatch_table_[0x17] = [this]{ op_fload();   }; dispatch_table_[0x22] = [this]{ op_fload_0(); };
    dispatch_table_[0x23] = [this]{ op_fload_1(); }; dispatch_table_[0x24] = [this]{ op_fload_2(); };
    dispatch_table_[0x25] = [this]{ op_fload_3(); };
    dispatch_table_[0x18] = [this]{ op_dload();   }; dispatch_table_[0x26] = [this]{ op_dload_0(); };
    dispatch_table_[0x27] = [this]{ op_dload_1(); }; dispatch_table_[0x28] = [this]{ op_dload_2(); };
    dispatch_table_[0x29] = [this]{ op_dload_3(); };
    dispatch_table_[0x19] = [this]{ op_aload();   }; dispatch_table_[0x2a] = [this]{ op_aload_0(); };
    dispatch_table_[0x2b] = [this]{ op_aload_1(); }; dispatch_table_[0x2c] = [this]{ op_aload_2(); };
    dispatch_table_[0x2d] = [this]{ op_aload_3(); };

    dispatch_table_[0x36] = [this]{ op_istore();   }; dispatch_table_[0x3b] = [this]{ op_istore_0(); };
    dispatch_table_[0x3c] = [this]{ op_istore_1(); }; dispatch_table_[0x3d] = [this]{ op_istore_2(); };
    dispatch_table_[0x3e] = [this]{ op_istore_3(); };
    dispatch_table_[0x37] = [this]{ op_lstore();   }; dispatch_table_[0x3f] = [this]{ op_lstore_0(); };
    dispatch_table_[0x40] = [this]{ op_lstore_1(); }; dispatch_table_[0x41] = [this]{ op_lstore_2(); };
    dispatch_table_[0x42] = [this]{ op_lstore_3(); };
    dispatch_table_[0x38] = [this]{ op_fstore();   }; dispatch_table_[0x43] = [this]{ op_fstore_0(); };
    dispatch_table_[0x44] = [this]{ op_fstore_1(); }; dispatch_table_[0x45] = [this]{ op_fstore_2(); };
    dispatch_table_[0x46] = [this]{ op_fstore_3(); };
    dispatch_table_[0x39] = [this]{ op_dstore();   }; dispatch_table_[0x47] = [this]{ op_dstore_0(); };
    dispatch_table_[0x48] = [this]{ op_dstore_1(); }; dispatch_table_[0x49] = [this]{ op_dstore_2(); };
    dispatch_table_[0x4a] = [this]{ op_dstore_3(); };
    dispatch_table_[0x3a] = [this]{ op_astore();   }; dispatch_table_[0x4b] = [this]{ op_astore_0(); };
    dispatch_table_[0x4c] = [this]{ op_astore_1(); }; dispatch_table_[0x4d] = [this]{ op_astore_2(); };
    dispatch_table_[0x4e] = [this]{ op_astore_3(); };

    dispatch_table_[0x60] = [this]{ op_iadd();  }; dispatch_table_[0x64] = [this]{ op_isub();  };
    dispatch_table_[0x68] = [this]{ op_imul();  }; dispatch_table_[0x6c] = [this]{ op_idiv();  };
    dispatch_table_[0x70] = [this]{ op_irem();  }; dispatch_table_[0x74] = [this]{ op_ineg();  };
    dispatch_table_[0x7e] = [this]{ op_iand();  }; dispatch_table_[0x80] = [this]{ op_ior();   };
    dispatch_table_[0x82] = [this]{ op_ixor();  }; dispatch_table_[0x78] = [this]{ op_ishl();  };
    dispatch_table_[0x7a] = [this]{ op_ishr();  }; dispatch_table_[0x7c] = [this]{ op_iushr(); };
    dispatch_table_[0x84] = [this]{ op_iinc();  };

    dispatch_table_[0x61] = [this]{ op_ladd();  }; dispatch_table_[0x65] = [this]{ op_lsub();  };
    dispatch_table_[0x69] = [this]{ op_lmul();  }; dispatch_table_[0x6d] = [this]{ op_ldiv();  };
    dispatch_table_[0x71] = [this]{ op_lrem();  }; dispatch_table_[0x75] = [this]{ op_lneg();  };
    dispatch_table_[0x7f] = [this]{ op_land();  }; dispatch_table_[0x81] = [this]{ op_lor();   };
    dispatch_table_[0x83] = [this]{ op_lxor();  }; dispatch_table_[0x79] = [this]{ op_lshl();  };
    dispatch_table_[0x7b] = [this]{ op_lshr();  }; dispatch_table_[0x7d] = [this]{ op_lushr(); };
    dispatch_table_[0x94] = [this]{ op_lcmp();  };

    dispatch_table_[0x62] = [this]{ op_fadd();  }; dispatch_table_[0x66] = [this]{ op_fsub();  };
    dispatch_table_[0x6a] = [this]{ op_fmul();  }; dispatch_table_[0x6e] = [this]{ op_fdiv();  };
    dispatch_table_[0x72] = [this]{ op_frem();  }; dispatch_table_[0x76] = [this]{ op_fneg();  };
    dispatch_table_[0x95] = [this]{ op_fcmpl(); }; dispatch_table_[0x96] = [this]{ op_fcmpg(); };

    dispatch_table_[0x63] = [this]{ op_dadd();  }; dispatch_table_[0x67] = [this]{ op_dsub();  };
    dispatch_table_[0x6b] = [this]{ op_dmul();  }; dispatch_table_[0x6f] = [this]{ op_ddiv();  };
    dispatch_table_[0x73] = [this]{ op_drem();  }; dispatch_table_[0x77] = [this]{ op_dneg();  };
    dispatch_table_[0x97] = [this]{ op_dcmpl(); }; dispatch_table_[0x98] = [this]{ op_dcmpg(); };

    dispatch_table_[0x57] = [this]{ op_pop();    }; dispatch_table_[0x58] = [this]{ op_pop2();   };
    dispatch_table_[0x59] = [this]{ op_dup();    }; dispatch_table_[0x5a] = [this]{ op_dup_x1(); };
    dispatch_table_[0x5b] = [this]{ op_dup_x2(); }; dispatch_table_[0x5c] = [this]{ op_dup2();   };
    dispatch_table_[0x5f] = [this]{ op_swap();   };

    dispatch_table_[0x85] = [this]{ op_i2l(); }; dispatch_table_[0x86] = [this]{ op_i2f(); };
    dispatch_table_[0x87] = [this]{ op_i2d(); }; dispatch_table_[0x88] = [this]{ op_l2i(); };
    dispatch_table_[0x89] = [this]{ op_l2f(); }; dispatch_table_[0x8a] = [this]{ op_l2d(); };
    dispatch_table_[0x8b] = [this]{ op_f2i(); }; dispatch_table_[0x8c] = [this]{ op_f2l(); };
    dispatch_table_[0x8d] = [this]{ op_f2d(); }; dispatch_table_[0x8e] = [this]{ op_d2i(); };
    dispatch_table_[0x8f] = [this]{ op_d2l(); }; dispatch_table_[0x90] = [this]{ op_d2f(); };
    dispatch_table_[0x91] = [this]{ op_i2b(); }; dispatch_table_[0x92] = [this]{ op_i2c(); };
    dispatch_table_[0x93] = [this]{ op_i2s(); };

    dispatch_table_[0x99] = [this]{ op_ifeq();      }; dispatch_table_[0x9a] = [this]{ op_ifne();      };
    dispatch_table_[0x9b] = [this]{ op_iflt();      }; dispatch_table_[0x9c] = [this]{ op_ifge();      };
    dispatch_table_[0x9d] = [this]{ op_ifgt();      }; dispatch_table_[0x9e] = [this]{ op_ifle();      };
    dispatch_table_[0x9f] = [this]{ op_if_icmpeq(); }; dispatch_table_[0xa0] = [this]{ op_if_icmpne(); };
    dispatch_table_[0xa1] = [this]{ op_if_icmplt(); }; dispatch_table_[0xa2] = [this]{ op_if_icmpge(); };
    dispatch_table_[0xa3] = [this]{ op_if_icmpgt(); }; dispatch_table_[0xa4] = [this]{ op_if_icmple(); };
    dispatch_table_[0xc6] = [this]{ op_ifnull();    }; dispatch_table_[0xc7] = [this]{ op_ifnonnull(); };
    dispatch_table_[0xa5] = [this]{ op_if_acmpeq(); }; dispatch_table_[0xa6] = [this]{ op_if_acmpne(); };
    dispatch_table_[0xa7] = [this]{ op_goto();      }; dispatch_table_[0xc8] = [this]{ op_goto_w();    };
    dispatch_table_[0xaa] = [this]{ op_tableswitch();  };
    dispatch_table_[0xab] = [this]{ op_lookupswitch(); };

    dispatch_table_[0xb8] = [this]{ op_invokestatic();    };
    dispatch_table_[0xb7] = [this]{ op_invokespecial();   };
    dispatch_table_[0xb6] = [this]{ op_invokevirtual();   };
    dispatch_table_[0xb9] = [this]{ op_invokeinterface(); };

    dispatch_table_[0xb1] = [this]{ op_return();  };
    dispatch_table_[0xac] = [this]{ op_ireturn(); }; dispatch_table_[0xad] = [this]{ op_lreturn(); };
    dispatch_table_[0xae] = [this]{ op_freturn(); }; dispatch_table_[0xaf] = [this]{ op_dreturn(); };
    dispatch_table_[0xb0] = [this]{ op_areturn(); };

    dispatch_table_[0xb2] = [this]{ op_getstatic(); }; dispatch_table_[0xb3] = [this]{ op_putstatic(); };
    dispatch_table_[0xb4] = [this]{ op_getfield();  }; dispatch_table_[0xb5] = [this]{ op_putfield();  };

    dispatch_table_[0xbb] = [this]{ op_new();         };
    dispatch_table_[0xbc] = [this]{ op_newarray();    }; dispatch_table_[0xbd] = [this]{ op_anewarray();  };
    dispatch_table_[0xbe] = [this]{ op_arraylength(); };
    dispatch_table_[0x2e] = [this]{ op_iaload();  }; dispatch_table_[0x4f] = [this]{ op_iastore(); };
    dispatch_table_[0x2f] = [this]{ op_laload();  }; dispatch_table_[0x50] = [this]{ op_lastore(); };
    dispatch_table_[0x30] = [this]{ op_faload();  }; dispatch_table_[0x51] = [this]{ op_fastore(); };
    dispatch_table_[0x31] = [this]{ op_daload();  }; dispatch_table_[0x52] = [this]{ op_dastore(); };
    dispatch_table_[0x32] = [this]{ op_aaload();  }; dispatch_table_[0x53] = [this]{ op_aastore(); };
    dispatch_table_[0x33] = [this]{ op_baload();  }; dispatch_table_[0x54] = [this]{ op_bastore(); };
    dispatch_table_[0x34] = [this]{ op_caload();  }; dispatch_table_[0x55] = [this]{ op_castore(); };
    dispatch_table_[0x35] = [this]{ op_saload();  }; dispatch_table_[0x56] = [this]{ op_sastore(); };

    dispatch_table_[0xbf] = [this]{ op_athrow();     };
    dispatch_table_[0xc0] = [this]{ op_checkcast();  };
    dispatch_table_[0xc1] = [this]{ op_instanceof(); };
}

void Interpreter::execute(const class_info& cls, const method_info& method) {
    const code_attribute* code = findCode(method);
    if (!code)
        throw std::runtime_error("Interpretador: metodo nao tem atributo Code");

    Frame f;
    f.cls    = &cls;
    f.method = &method;
    f.code   = code;
    f.pc     = 0;
    f.locals.resize(code->max_locals, Value::fromInt(0));

    frame_stack_.push(std::move(f));
    run();
}

void Interpreter::run() {
    while (!halted_ && !frame_stack_.empty()) {
        currentFrame().last_pc = currentFrame().pc;
        uint8_t opcode = fetchU1();

        auto it = dispatch_table_.find(opcode);
        if (it == dispatch_table_.end()) {
            std::ostringstream oss;
            oss << "Opcode nao implementado: 0x"
                << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(opcode);
            throw std::runtime_error(oss.str());
        }

        try {
            it->second();
        }
        catch (const JavaException& je) {
            if (!dispatchException(frame_stack_, loader_, je.ref, je.class_name))
                throw std::runtime_error("Exception in thread \"main\" " + je.class_name);
        }
        catch (const std::runtime_error& re) {
            std::string cls = builtinClassForMessage(re.what());
            if (cls.empty())
                throw;
            int32_t ref = heap_.allocateObject(nullptr, cls);
            if (!dispatchException(frame_stack_, loader_, ref, cls))
                throw;
        }
    }
}

Frame& Interpreter::currentFrame() {
    return frame_stack_.top();
}

u1 Interpreter::fetchU1() {
    Frame& f = currentFrame();
    return f.code->code[f.pc++];
}

u2 Interpreter::fetchU2() {
    u1 hi = fetchU1();
    u1 lo = fetchU1();
    return static_cast<u2>((hi << 8) | lo);
}

u4 Interpreter::fetchU4() {
    u2 hi = fetchU2();
    u2 lo = fetchU2();
    return static_cast<u4>((hi << 16) | lo);
}

int8_t  Interpreter::fetchS1() { return static_cast<int8_t> (fetchU1()); }
int16_t Interpreter::fetchS2() { return static_cast<int16_t>(fetchU2()); }
int32_t Interpreter::fetchS4() { return static_cast<int32_t>(fetchU4()); }

void Interpreter::branch(int32_t offset) {
    Frame& f = currentFrame();
    f.pc += offset - 3;
}

const code_attribute* Interpreter::findCode(const method_info& method) const {
    for (const attribute_info& attr : method.attributes)
        if (attr.code_data)
            return attr.code_data.get();
    return nullptr;
}

void Interpreter::op_nop() {}

void Interpreter::op_aconst_null() {
    currentFrame().push(Value::null());
}

void Interpreter::op_iconst(int32_t value) {
    currentFrame().push(Value::fromInt(value));
}

void Interpreter::op_iconst_m1() { op_iconst(-1); }
void Interpreter::op_iconst_0()  { op_iconst(0);  }
void Interpreter::op_iconst_1()  { op_iconst(1);  }
void Interpreter::op_iconst_2()  { op_iconst(2);  }
void Interpreter::op_iconst_3()  { op_iconst(3);  }
void Interpreter::op_iconst_4()  { op_iconst(4);  }
void Interpreter::op_iconst_5()  { op_iconst(5);  }

void Interpreter::op_lconst_0() { currentFrame().push(Value::fromLong(0)); }
void Interpreter::op_lconst_1() { currentFrame().push(Value::fromLong(1)); }

void Interpreter::op_fconst_0() { currentFrame().push(Value::fromFloat(0.0f)); }
void Interpreter::op_fconst_1() { currentFrame().push(Value::fromFloat(1.0f)); }
void Interpreter::op_fconst_2() { currentFrame().push(Value::fromFloat(2.0f)); }

void Interpreter::op_dconst_0() { currentFrame().push(Value::fromDouble(0.0)); }
void Interpreter::op_dconst_1() { currentFrame().push(Value::fromDouble(1.0)); }

void Interpreter::op_bipush() {
    int8_t val = fetchS1();
    currentFrame().push(Value::fromInt(val));
}

void Interpreter::op_sipush() {
    int16_t val = fetchS2();
    currentFrame().push(Value::fromInt(val));
}

void Interpreter::op_iadd() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    f.push(Value::fromInt(a + b));
}

void Interpreter::op_isub() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    f.push(Value::fromInt(a - b));
}

void Interpreter::op_imul() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    f.push(Value::fromInt(a * b));
}

void Interpreter::op_idiv() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    if (b == 0) throw std::runtime_error("ArithmeticException: / by zero");
    f.push(Value::fromInt(a / b));
}

void Interpreter::op_irem() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    if (b == 0) throw std::runtime_error("ArithmeticException: / by zero");
    f.push(Value::fromInt(a % b));
}

void Interpreter::op_ineg() {
    Frame& f = currentFrame();
    f.push(Value::fromInt(-f.pop().data.i));
}

void Interpreter::op_iload_0() { Frame& f = currentFrame(); f.push(f.locals[0]); }
void Interpreter::op_iload_1() { Frame& f = currentFrame(); f.push(f.locals[1]); }
void Interpreter::op_iload_2() { Frame& f = currentFrame(); f.push(f.locals[2]); }
void Interpreter::op_iload_3() { Frame& f = currentFrame(); f.push(f.locals[3]); }

void Interpreter::op_iload() {
    Frame& f = currentFrame();
    u1 idx = fetchU1();
    f.push(f.locals[idx]);
}

void Interpreter::op_istore_0() { Frame& f = currentFrame(); f.locals[0] = f.pop(); }
void Interpreter::op_istore_1() { Frame& f = currentFrame(); f.locals[1] = f.pop(); }
void Interpreter::op_istore_2() { Frame& f = currentFrame(); f.locals[2] = f.pop(); }
void Interpreter::op_istore_3() { Frame& f = currentFrame(); f.locals[3] = f.pop(); }

void Interpreter::op_istore() {
    Frame& f = currentFrame();
    u1 idx = fetchU1();
    f.locals[idx] = f.pop();
}

void Interpreter::op_iinc() {
    Frame& f  = currentFrame();
    u1     idx = fetchU1();
    int8_t cst = fetchS1();
    f.locals[idx].data.i += cst;
}

void Interpreter::op_return() {
    frame_stack_.pop();
}

void Interpreter::op_ireturn() {
    Value retval = frame_stack_.top().pop();
    frame_stack_.pop();
    if (!frame_stack_.empty())
        frame_stack_.top().push(retval);
}

void Interpreter::op_goto() {
    int16_t offset = fetchS2();
    branch(offset);
}
void Interpreter::op_ldc() {
    Frame& f = currentFrame();
    u1 idx = fetchU1();
    const cp_info& entry = f.cls->constant_pool[idx];

    if (entry.tag == CONSTANT_Integer) {
        int32_t val = static_cast<int32_t>(entry.container.Integer.bytes);
        f.push(Value::fromInt(val));
    }
    else if (entry.tag == CONSTANT_Float) {
        float val;
        u4 bits = entry.container.Float.bytes;
        memcpy(&val, &bits, sizeof(float));
        f.push(Value::fromFloat(val));
    }
    else if (entry.tag == CONSTANT_String) {
        f.push(Value::fromRef(static_cast<int32_t>(idx)));
    }
}

void Interpreter::op_ldc_w() {
    Frame& f = currentFrame();
    u2 idx = fetchU2();
    const cp_info& entry = f.cls->constant_pool[idx];

    if (entry.tag == CONSTANT_Integer) {
        int32_t val = static_cast<int32_t>(entry.container.Integer.bytes);
        f.push(Value::fromInt(val));
    }
    else if (entry.tag == CONSTANT_Float) {
        float val;
        u4 bits = entry.container.Float.bytes;
        memcpy(&val, &bits, sizeof(float));
        f.push(Value::fromFloat(val));
    }
    else if (entry.tag == CONSTANT_String) {
        f.push(Value::fromRef(static_cast<int32_t>(idx)));
    }
}

void Interpreter::op_ldc2_w() {
    Frame& f = currentFrame();
    u2 idx = fetchU2();
    const cp_info& entry = f.cls->constant_pool[idx];

    if (entry.tag == CONSTANT_Long) {
        int64_t val = (static_cast<int64_t>(entry.container.Long.high_bytes) << 32)
                    |  entry.container.Long.low_bytes;
        f.push(Value::fromLong(val));
    }
    else if (entry.tag == CONSTANT_Double) {
        u8 bits = (static_cast<u8>(entry.container.Double.high_bytes) << 32)
                |  entry.container.Double.low_bytes;
        double val;
        memcpy(&val, &bits, sizeof(double));
        f.push(Value::fromDouble(val));
    }
}

void Interpreter::op_ladd() {
    Frame& f = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    int64_t result = a + b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_lmul() {
    Frame& f = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    int64_t result = a * b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_lsub() {
    Frame& f = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    int64_t result = a - b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_ldiv() {
    Frame& f = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    if (b == 0) throw std::runtime_error("ArithmeticException: / by zero");
    int64_t result = a / b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_lrem() {
    Frame& f = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    if (b == 0) throw std::runtime_error("ArithmeticException: / by zero");
    int64_t result = a % b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_lneg() {
    Frame& f = currentFrame();
    int64_t a = f.pop().data.l;
    int64_t result = -a;
    f.push(Value::fromLong(result));
}

void Interpreter::op_land() {
    Frame& f = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    int64_t result = a & b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_lor() {
    Frame& f = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    int64_t result = a | b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_lxor() {
    Frame& f = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    int64_t result = a ^ b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_lshl() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i & 0x3f;
    int64_t a = f.pop().data.l;
    int64_t result = a << b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_lshr() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i & 0x3f;
    int64_t a = f.pop().data.l;
    int64_t result = a >> b;
    f.push(Value::fromLong(result));
}

void Interpreter::op_lushr() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i & 0x3f;
    u8      a = static_cast<u8>(f.pop().data.l);
    int64_t result = static_cast<int64_t>(a >> b);
    f.push(Value::fromLong(result));
}

void Interpreter::op_fadd() {
    Frame& f = currentFrame();
    float b = f.pop().data.f;
    float a = f.pop().data.f;
    float result = a + b;
    f.push(Value::fromFloat(result));
}

void Interpreter::op_fsub() {
    Frame& f = currentFrame();
    float b = f.pop().data.f;
    float a = f.pop().data.f;
    float result = a - b;
    f.push(Value::fromFloat(result));
}

void Interpreter::op_fmul() {
    Frame& f = currentFrame();
    float b = f.pop().data.f;
    float a = f.pop().data.f;
    float result = a * b;
    f.push(Value::fromFloat(result));
}

void Interpreter::op_fdiv() {
    Frame& f = currentFrame();
    float b = f.pop().data.f;
    float a = f.pop().data.f;
    float result = a / b;
    f.push(Value::fromFloat(result));
}

void Interpreter::op_fneg() {
    Frame& f = currentFrame();
    float a = f.pop().data.f;
    float result = -a;
    f.push(Value::fromFloat(result));
}

void Interpreter::op_frem() {
    Frame& f = currentFrame();
    float b = f.pop().data.f;
    float a = f.pop().data.f;
    float result = std::fmod(a, b);
    f.push(Value::fromFloat(result));
}

void Interpreter::op_dadd() { 
    Frame& f = currentFrame();
    double b = f.pop().data.d;
    double a = f.pop().data.d;
    double result = a + b;
    f.push(Value::fromDouble(result));
}

void Interpreter::op_dsub() { 
    Frame& f = currentFrame();
    double b = f.pop().data.d;
    double a = f.pop().data.d;
    double result = a - b;
    f.push(Value::fromDouble(result));
}

void Interpreter::op_dmul() { 
    Frame& f = currentFrame();
    double b = f.pop().data.d;
    double a = f.pop().data.d;
    double result = a * b;
    f.push(Value::fromDouble(result));
}

void Interpreter::op_ddiv() { 
    Frame& f = currentFrame();
    double b = f.pop().data.d;
    double a = f.pop().data.d;
    double result = a / b;
    f.push(Value::fromDouble(result));
}

void Interpreter::op_dneg() {
    Frame& f = currentFrame();
    double a = f.pop().data.d;
    double result = -a;
    f.push(Value::fromDouble(result));
}

void Interpreter::op_drem() {
    Frame& f = currentFrame();
    double b = f.pop().data.d;
    double a = f.pop().data.d;
    double result = std::fmod(a, b);
    f.push(Value::fromDouble(result));
}

void Interpreter::op_lcmp() {
    Frame& f  = currentFrame();
    int64_t b = f.pop().data.l;
    int64_t a = f.pop().data.l;
    int32_t result = (a > b ? 1 : (a < b ? -1 : 0));
    f.push(Value::fromInt(result));
}

void Interpreter::op_fcmpl() {
    Frame& f = currentFrame();
    float b  = f.pop().data.f;
    float a  = f.pop().data.f;
    if (std::isnan(a) || std::isnan(b)) {
        f.push(Value::fromInt(-1));
        return;
    }
    int32_t result = (a > b ? 1 : (a < b ? -1 : 0));
    f.push(Value::fromInt(result));
}

void Interpreter::op_fcmpg() {
    Frame& f = currentFrame();
    float b  = f.pop().data.f;
    float a  = f.pop().data.f;
    if (std::isnan(a) || std::isnan(b)) {
        f.push(Value::fromInt(1));
        return;
    }
    int32_t result = (a > b ? 1 : (a < b ? -1 : 0));
    f.push(Value::fromInt(result));
}

void Interpreter::op_dcmpl() {
    Frame& f = currentFrame();
    double b = f.pop().data.d;
    double a = f.pop().data.d;
    if (std::isnan(a) || std::isnan(b)) {
        f.push(Value::fromInt(-1));
        return;
    }
    int32_t result = (a > b ? 1 : (a < b ? -1 : 0));
    f.push(Value::fromInt(result));
}

void Interpreter::op_dcmpg() {
    Frame& f = currentFrame();
    double b = f.pop().data.d;
    double a = f.pop().data.d;
    if (std::isnan(a) || std::isnan(b)) {
        f.push(Value::fromInt(1));
        return;
    }
    int32_t result = (a > b ? 1 : (a < b ? -1 : 0));
    f.push(Value::fromInt(result));
}

void Interpreter::op_lload_0() { Frame& f = currentFrame(); f.push(f.locals[0]); }
void Interpreter::op_lload_1() { Frame& f = currentFrame(); f.push(f.locals[1]); }
void Interpreter::op_lload_2() { Frame& f = currentFrame(); f.push(f.locals[2]); }
void Interpreter::op_lload_3() { Frame& f = currentFrame(); f.push(f.locals[3]); }
void Interpreter::op_lload()   { Frame& f = currentFrame(); u1 idx = fetchU1(); f.push(f.locals[idx]); }

void Interpreter::op_fload_0() { Frame& f = currentFrame(); f.push(f.locals[0]); }
void Interpreter::op_fload_1() { Frame& f = currentFrame(); f.push(f.locals[1]); }
void Interpreter::op_fload_2() { Frame& f = currentFrame(); f.push(f.locals[2]); }
void Interpreter::op_fload_3() { Frame& f = currentFrame(); f.push(f.locals[3]); }
void Interpreter::op_fload()   { Frame& f = currentFrame(); u1 idx = fetchU1(); f.push(f.locals[idx]); }

void Interpreter::op_dload_0() { Frame& f = currentFrame(); f.push(f.locals[0]); }
void Interpreter::op_dload_1() { Frame& f = currentFrame(); f.push(f.locals[1]); }
void Interpreter::op_dload_2() { Frame& f = currentFrame(); f.push(f.locals[2]); }
void Interpreter::op_dload_3() { Frame& f = currentFrame(); f.push(f.locals[3]); }
void Interpreter::op_dload()   { Frame& f = currentFrame(); u1 idx = fetchU1(); f.push(f.locals[idx]); }

void Interpreter::op_aload_0() { Frame& f = currentFrame(); f.push(f.locals[0]); }
void Interpreter::op_aload_1() { Frame& f = currentFrame(); f.push(f.locals[1]); }
void Interpreter::op_aload_2() { Frame& f = currentFrame(); f.push(f.locals[2]); }
void Interpreter::op_aload_3() { Frame& f = currentFrame(); f.push(f.locals[3]); }
void Interpreter::op_aload()   { Frame& f = currentFrame(); u1 idx = fetchU1(); f.push(f.locals[idx]); }

void Interpreter::op_lstore_0() { Frame& f = currentFrame(); f.locals[0] = f.pop(); }
void Interpreter::op_lstore_1() { Frame& f = currentFrame(); f.locals[1] = f.pop(); }
void Interpreter::op_lstore_2() { Frame& f = currentFrame(); f.locals[2] = f.pop(); }
void Interpreter::op_lstore_3() { Frame& f = currentFrame(); f.locals[3] = f.pop(); }
void Interpreter::op_lstore()   { Frame& f = currentFrame(); u1 idx = fetchU1(); f.locals[idx] = f.pop(); }

void Interpreter::op_fstore_0() { Frame& f = currentFrame(); f.locals[0] = f.pop(); }
void Interpreter::op_fstore_1() { Frame& f = currentFrame(); f.locals[1] = f.pop(); }
void Interpreter::op_fstore_2() { Frame& f = currentFrame(); f.locals[2] = f.pop(); }
void Interpreter::op_fstore_3() { Frame& f = currentFrame(); f.locals[3] = f.pop(); }
void Interpreter::op_fstore()   { Frame& f = currentFrame(); u1 idx = fetchU1(); f.locals[idx] = f.pop(); }

void Interpreter::op_dstore_0() { Frame& f = currentFrame(); f.locals[0] = f.pop(); }
void Interpreter::op_dstore_1() { Frame& f = currentFrame(); f.locals[1] = f.pop(); }
void Interpreter::op_dstore_2() { Frame& f = currentFrame(); f.locals[2] = f.pop(); }
void Interpreter::op_dstore_3() { Frame& f = currentFrame(); f.locals[3] = f.pop(); }
void Interpreter::op_dstore()   { Frame& f = currentFrame(); u1 idx = fetchU1(); f.locals[idx] = f.pop(); }

void Interpreter::op_astore_0() { Frame& f = currentFrame(); f.locals[0] = f.pop(); }
void Interpreter::op_astore_1() { Frame& f = currentFrame(); f.locals[1] = f.pop(); }
void Interpreter::op_astore_2() { Frame& f = currentFrame(); f.locals[2] = f.pop(); }
void Interpreter::op_astore_3() { Frame& f = currentFrame(); f.locals[3] = f.pop(); }
void Interpreter::op_astore()   { Frame& f = currentFrame(); u1 idx = fetchU1(); f.locals[idx] = f.pop(); }

void Interpreter::op_iand() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    f.push(Value::fromInt(a & b));
}

void Interpreter::op_ior() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    f.push(Value::fromInt(a | b));
}

void Interpreter::op_ixor() {
    Frame& f = currentFrame();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    f.push(Value::fromInt(a ^ b));
}

void Interpreter::op_ishl() {
    Frame& f = currentFrame();
    int32_t shift = f.pop().data.i & 0x1f;
    int32_t value = f.pop().data.i;
    uint32_t result = static_cast<uint32_t>(value) << shift;
    f.push(Value::fromInt(static_cast<int32_t>(result)));
}

void Interpreter::op_ishr() {
    Frame& f = currentFrame();
    int32_t shift = f.pop().data.i & 0x1f;
    int32_t value = f.pop().data.i;
    f.push(Value::fromInt(value >> shift));
}

void Interpreter::op_iushr() {
    Frame& f = currentFrame();
    int32_t shift = f.pop().data.i & 0x1f;
    uint32_t value = static_cast<uint32_t>(f.pop().data.i);
    f.push(Value::fromInt(static_cast<int32_t>(value >> shift)));
}
namespace {
bool isCategory2(const Value& value) {
    return value.type == Value::Type::LONG || value.type == Value::Type::DOUBLE;
}

void requireCategory1(const Value& value, const char* opcode) {
    if (isCategory2(value))
        throw std::runtime_error(std::string(opcode) + ": operando category 2 invalido para esta forma");
}
} // namespace

void Interpreter::op_pop() {
    Frame& f = currentFrame();
    requireCategory1(f.top(), "pop");
    f.pop();
}

void Interpreter::op_pop2() {
    Frame& f = currentFrame();
    Value value1 = f.pop();
    if (isCategory2(value1))
        return;

    Value value2 = f.pop();
    requireCategory1(value2, "pop2");
}

void Interpreter::op_dup() {
    Frame& f = currentFrame();
    Value value = f.top();
    requireCategory1(value, "dup");
    f.push(value);
}

void Interpreter::op_dup_x1() {
    Frame& f = currentFrame();
    Value value1 = f.pop();
    Value value2 = f.pop();
    requireCategory1(value1, "dup_x1");
    requireCategory1(value2, "dup_x1");

    f.push(value1);
    f.push(value2);
    f.push(value1);
}

void Interpreter::op_dup_x2() {
    Frame& f = currentFrame();
    Value value1 = f.pop();
    Value value2 = f.pop();
    requireCategory1(value1, "dup_x2");

    if (isCategory2(value2)) {
        f.push(value1);
        f.push(value2);
        f.push(value1);
        return;
    }

    Value value3 = f.pop();
    requireCategory1(value3, "dup_x2");

    f.push(value1);
    f.push(value3);
    f.push(value2);
    f.push(value1);
}

void Interpreter::op_dup2() {
    Frame& f = currentFrame();
    Value value1 = f.pop();
    if (isCategory2(value1)) {
        f.push(value1);
        f.push(value1);
        return;
    }

    Value value2 = f.pop();
    requireCategory1(value2, "dup2");

    f.push(value2);
    f.push(value1);
    f.push(value2);
    f.push(value1);
}

void Interpreter::op_swap() {
    Frame& f = currentFrame();
    Value value1 = f.pop();
    Value value2 = f.pop();
    requireCategory1(value1, "swap");
    requireCategory1(value2, "swap");

    f.push(value1);
    f.push(value2);
}

namespace {
int32_t floatToInt(float v) {
    if (std::isnan(v)) return 0;
    if (v >= 2147483647.0f)  return INT32_MAX;
    if (v <= -2147483648.0f) return INT32_MIN;
    return static_cast<int32_t>(v);
}
int64_t floatToLong(float v) {
    if (std::isnan(v)) return 0;
    if (v >= 9223372036854775807.0f)  return INT64_MAX;
    if (v <= -9223372036854775808.0f) return INT64_MIN;
    return static_cast<int64_t>(v);
}
int32_t doubleToInt(double v) {
    if (std::isnan(v)) return 0;
    if (v >= 2147483647.0)  return INT32_MAX;
    if (v <= -2147483648.0) return INT32_MIN;
    return static_cast<int32_t>(v);
}
int64_t doubleToLong(double v) {
    if (std::isnan(v)) return 0;
    if (v >= 9223372036854775807.0)  return INT64_MAX;
    if (v <= -9223372036854775808.0) return INT64_MIN;
    return static_cast<int64_t>(v);
}
} // namespace

void Interpreter::op_i2l() {
    Frame& f = currentFrame();
    int32_t v = f.pop().data.i;
    f.push(Value::fromLong(static_cast<int64_t>(v)));
}

void Interpreter::op_i2f() {
    Frame& f = currentFrame();
    int32_t v = f.pop().data.i;
    f.push(Value::fromFloat(static_cast<float>(v)));
}

void Interpreter::op_i2d() {
    Frame& f = currentFrame();
    int32_t v = f.pop().data.i;
    f.push(Value::fromDouble(static_cast<double>(v)));
}

void Interpreter::op_l2i() {
    Frame& f = currentFrame();
    int64_t v = f.pop().data.l;
    f.push(Value::fromInt(static_cast<int32_t>(v)));
}

void Interpreter::op_l2f() {
    Frame& f = currentFrame();
    int64_t v = f.pop().data.l;
    f.push(Value::fromFloat(static_cast<float>(v)));
}

void Interpreter::op_l2d() {
    Frame& f = currentFrame();
    int64_t v = f.pop().data.l;
    f.push(Value::fromDouble(static_cast<double>(v)));
}

void Interpreter::op_f2i() {
    Frame& f = currentFrame();
    float v = f.pop().data.f;
    f.push(Value::fromInt(floatToInt(v)));
}

void Interpreter::op_f2l() {
    Frame& f = currentFrame();
    float v = f.pop().data.f;
    f.push(Value::fromLong(floatToLong(v)));
}

void Interpreter::op_f2d() {
    Frame& f = currentFrame();
    float v = f.pop().data.f;
    f.push(Value::fromDouble(static_cast<double>(v)));
}

void Interpreter::op_d2i() {
    Frame& f = currentFrame();
    double v = f.pop().data.d;
    f.push(Value::fromInt(doubleToInt(v)));
}

void Interpreter::op_d2l() {
    Frame& f = currentFrame();
    double v = f.pop().data.d;
    f.push(Value::fromLong(doubleToLong(v)));
}

void Interpreter::op_d2f() {
    Frame& f = currentFrame();
    double v = f.pop().data.d;
    f.push(Value::fromFloat(static_cast<float>(v)));
}

void Interpreter::op_i2b() {
    Frame& f = currentFrame();
    int32_t v = f.pop().data.i;
    f.push(Value::fromInt(static_cast<int32_t>(static_cast<int8_t>(v))));
}

void Interpreter::op_i2c() {
    Frame& f = currentFrame();
    int32_t v = f.pop().data.i;
    f.push(Value::fromInt(static_cast<int32_t>(static_cast<uint16_t>(v))));
}

void Interpreter::op_i2s() {
    Frame& f = currentFrame();
    int32_t v = f.pop().data.i;
    f.push(Value::fromInt(static_cast<int32_t>(static_cast<int16_t>(v))));
}

void Interpreter::op_ifeq() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    if (f.pop().data.i == 0) branch(offset);
}

void Interpreter::op_ifne() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    if (f.pop().data.i != 0) branch(offset);
}

void Interpreter::op_iflt() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    if (f.pop().data.i < 0) branch(offset);
}

void Interpreter::op_ifge() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    if (f.pop().data.i >= 0) branch(offset);
}

void Interpreter::op_ifgt() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    if (f.pop().data.i > 0) branch(offset);
}

void Interpreter::op_ifle() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    if (f.pop().data.i <= 0) branch(offset);
}

void Interpreter::op_if_icmpeq() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    if (a == b) branch(offset);
}

void Interpreter::op_if_icmpne() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    if (a != b) branch(offset);
}

void Interpreter::op_if_icmplt() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    if (a < b) branch(offset);
}

void Interpreter::op_if_icmpge() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    if (a >= b) branch(offset);
}

void Interpreter::op_if_icmpgt() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    if (a > b) branch(offset);
}

void Interpreter::op_if_icmple() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    int32_t b = f.pop().data.i;
    int32_t a = f.pop().data.i;
    if (a <= b) branch(offset);
}

void Interpreter::op_ifnull() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    if (heap_.isNull(f.pop().data.ref)) branch(offset);
}

void Interpreter::op_ifnonnull() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    if (!heap_.isNull(f.pop().data.ref)) branch(offset);
}

void Interpreter::op_if_acmpeq() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    int32_t b = f.pop().data.ref;
    int32_t a = f.pop().data.ref;
    if (a == b) branch(offset);
}

void Interpreter::op_if_acmpne() {
    Frame& f = currentFrame();
    int16_t offset = fetchS2();
    int32_t b = f.pop().data.ref;
    int32_t a = f.pop().data.ref;
    if (a != b) branch(offset);
}

void Interpreter::op_goto_w() {
    int32_t offset = fetchS4();
    Frame& f = currentFrame();
    f.pc += offset - 5;
}

void Interpreter::op_tableswitch() {
    Frame& f = currentFrame();
    size_t start = f.pc - 1;
    while (f.pc % 4 != 0) f.pc++;

    int32_t def  = fetchS4();
    int32_t low  = fetchS4();
    int32_t high = fetchS4();

    int32_t index = f.pop().data.i;

    int32_t offset;
    if (index < low || index > high) {
        offset = def;
    } else {
        f.pc += static_cast<size_t>(index - low) * 4;
        offset = fetchS4();
    }

    f.pc = start + static_cast<size_t>(offset);
}

void Interpreter::op_lookupswitch() {
    Frame& f = currentFrame();
    size_t start = f.pc - 1;
    while (f.pc % 4 != 0) f.pc++;

    int32_t def    = fetchS4();
    int32_t npairs = fetchS4();

    int32_t key = f.pop().data.i;
    int32_t offset = def;
    for (int32_t j = 0; j < npairs; j++) {
        int32_t match = fetchS4();
        int32_t off   = fetchS4();
        if (match == key) {
            offset = off;
            break;
        }
    }

    f.pc = start + static_cast<size_t>(offset);
}

void Interpreter::op_lreturn() {
    Value retval = frame_stack_.top().pop();
    frame_stack_.pop();
    if (!frame_stack_.empty())
        frame_stack_.top().push(retval);
}

void Interpreter::op_freturn() {
    Value retval = frame_stack_.top().pop();
    frame_stack_.pop();
    if (!frame_stack_.empty())
        frame_stack_.top().push(retval);
}

void Interpreter::op_dreturn() {
    Value retval = frame_stack_.top().pop();
    frame_stack_.pop();
    if (!frame_stack_.empty())
        frame_stack_.top().push(retval);
}

void Interpreter::op_areturn() {
    Value retval = frame_stack_.top().pop();
    frame_stack_.pop();
    if (!frame_stack_.empty())
        frame_stack_.top().push(retval);
}

namespace {

std::vector<int> parseParamWidths(const std::string& descriptor) {
    std::vector<int> widths;
    size_t i = 1; // pula '('
    while (i < descriptor.size() && descriptor[i] != ')') {
        char c = descriptor[i];
        if (c == '[') { i++; continue; }
        if (c == 'L') {
            while (i < descriptor.size() && descriptor[i] != ';') i++;
            i++;                                   // pula ';'
            widths.push_back(1);
            continue;
        }
        widths.push_back((c == 'J' || c == 'D') ? 2 : 1);
        i++;
    }
    return widths;
}
const code_attribute* findCodeOf(const method_info& method) {
    for (const attribute_info& attr : method.attributes)
        if (attr.code_data)
            return attr.code_data.get();
    return nullptr;
}

const method_info* resolveMethod(ClassLoader& loader, const class_info& start,
                                 const std::string& name, const std::string& descriptor,
                                 const class_info** decl) {
    const class_info* cls = &start;
    while (true) {
        if (const method_info* m = findMethod(*cls, name, descriptor)) {
            if (decl) *decl = cls;
            return m;
        }
        if (cls->super_class == 0)
            return nullptr;
        std::string super = classNameFromConstantPool(*cls, cls->super_class);
        try {
            cls = &loader.load(super);
        } catch (...) {
            return nullptr;
        }
    }
}

void pushCalleeFrame(FrameStack& fs, const class_info& decl, const method_info& method,
                     const code_attribute* code, const std::vector<Value>& args,
                     const std::vector<int>& widths, bool hasThis, Value self) {
    Frame nf;
    nf.cls    = &decl;
    nf.method = &method;
    nf.code   = code;
    nf.pc     = 0;
    nf.locals.assign(code->max_locals, Value::fromInt(0));

    int slot = 0;
    if (hasThis)
        nf.locals[slot++] = self;
    for (size_t k = 0; k < args.size(); k++) {
        nf.locals[slot] = args[k];
        slot += widths[k];
    }
    fs.push(std::move(nf));
}

void invokeStaticMethod(ClassLoader& loader, FrameStack& fs,
                        const std::string& class_name, const std::string& name,
                        const std::string& descriptor) {
    Frame& caller = fs.top();
    std::vector<int> widths = parseParamWidths(descriptor);
    std::vector<Value> args(widths.size());
    for (size_t k = widths.size(); k-- > 0; )
        args[k] = caller.pop();

    const class_info* target = nullptr;
    try { target = &loader.load(class_name); } catch (...) { target = nullptr; }

    const class_info* decl = nullptr;
    const method_info* m = target ? resolveMethod(loader, *target, name, descriptor, &decl) : nullptr;
    const code_attribute* code = m ? findCodeOf(*m) : nullptr;
    if (!m || !code)
        throw std::runtime_error("invokestatic: metodo nao encontrado ou sem Code: " +
                                 class_name + "." + name + descriptor);

    pushCalleeFrame(fs, *decl, *m, code, args, widths, false, Value::null());
}

void invokeInstanceMethod(ClassLoader& loader, FrameStack& fs, Heap& heap,
                          const std::string& static_class, const std::string& name,
                          const std::string& descriptor, bool virtualDispatch) {
    Frame& caller = fs.top();
    std::vector<int> widths = parseParamWidths(descriptor);
    std::vector<Value> args(widths.size());
    for (size_t k = widths.size(); k-- > 0; )
        args[k] = caller.pop();
    Value self = caller.pop();

    if (heap.isNull(self.data.ref))
        throw std::runtime_error("NullPointerException");

    std::string start_class = static_class;
    if (virtualDispatch) {
        try { start_class = heap.object(self.data.ref).class_name; } catch (...) {}
    }

    const class_info* target = nullptr;
    try { target = &loader.load(start_class); } catch (...) { target = nullptr; }

    const class_info* decl = nullptr;
    const method_info* m = target ? resolveMethod(loader, *target, name, descriptor, &decl) : nullptr;
    const code_attribute* code = m ? findCodeOf(*m) : nullptr;
    if (!m || !code)
        return;

    pushCalleeFrame(fs, *decl, *m, code, args, widths, true, self);
}

} // namespace

void Interpreter::op_invokestatic() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    const cp_info& ref  = f.cls->constant_pool[cp_index];
    std::string class_name  = classNameFromConstantPool(*f.cls, ref.container.Methodref.class_index);
    const cp_info& nat      = f.cls->constant_pool[ref.container.Methodref.name_and_type_index];
    std::string method_name = utf8FromConstantPool(*f.cls, nat.container.NameAndType.name_index);
    std::string descriptor  = utf8FromConstantPool(*f.cls, nat.container.NameAndType.descriptor_index);

    invokeStaticMethod(loader_, frame_stack_, class_name, method_name, descriptor);
}

void Interpreter::op_invokespecial() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    const cp_info& ref  = f.cls->constant_pool[cp_index];
    std::string class_name  = classNameFromConstantPool(*f.cls, ref.container.Methodref.class_index);
    const cp_info& nat      = f.cls->constant_pool[ref.container.Methodref.name_and_type_index];
    std::string method_name = utf8FromConstantPool(*f.cls, nat.container.NameAndType.name_index);
    std::string descriptor  = utf8FromConstantPool(*f.cls, nat.container.NameAndType.descriptor_index);

    invokeInstanceMethod(loader_, frame_stack_, heap_, class_name, method_name, descriptor, false);
}

void Interpreter::op_invokeinterface() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();
    fetchU1();
    fetchU1();

    const cp_info& ref  = f.cls->constant_pool[cp_index];
    std::string class_name  = classNameFromConstantPool(*f.cls, ref.container.InterfaceMethodref.class_index);
    const cp_info& nat      = f.cls->constant_pool[ref.container.InterfaceMethodref.name_and_type_index];
    std::string method_name = utf8FromConstantPool(*f.cls, nat.container.NameAndType.name_index);
    std::string descriptor  = utf8FromConstantPool(*f.cls, nat.container.NameAndType.descriptor_index);

    invokeInstanceMethod(loader_, frame_stack_, heap_, class_name, method_name, descriptor, true);
}

void Interpreter::op_getfield() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    const cp_info& ref  = f.cls->constant_pool[cp_index];
    const cp_info& nat  = f.cls->constant_pool[ref.container.Fieldref.name_and_type_index];
    std::string field_name = utf8FromConstantPool(*f.cls, nat.container.NameAndType.name_index);

    int32_t objref = f.pop().data.ref;
    if (heap_.isNull(objref))
        throw std::runtime_error("NullPointerException");
    f.push(heap_.getField(objref, field_name));
}

void Interpreter::op_putfield() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    const cp_info& ref  = f.cls->constant_pool[cp_index];
    const cp_info& nat  = f.cls->constant_pool[ref.container.Fieldref.name_and_type_index];
    std::string field_name = utf8FromConstantPool(*f.cls, nat.container.NameAndType.name_index);

    Value   value  = f.pop();
    int32_t objref = f.pop().data.ref;
    if (heap_.isNull(objref))
        throw std::runtime_error("NullPointerException");
    heap_.putField(objref, field_name, value);
}

void Interpreter::op_getstatic() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    const cp_info& ref  = f.cls->constant_pool[cp_index];
    std::string class_name = classNameFromConstantPool(*f.cls, ref.container.Fieldref.class_index);
    const cp_info& nat     = f.cls->constant_pool[ref.container.Fieldref.name_and_type_index];
    std::string field_name = utf8FromConstantPool(*f.cls, nat.container.NameAndType.name_index);
    std::string descriptor = utf8FromConstantPool(*f.cls, nat.container.NameAndType.descriptor_index);

    if (class_name == "java/lang/System" && field_name == "out") {
        f.push(Value::null());
        return;
    }

    std::string key = class_name + "::" + field_name;
    if (!loader_.hasStaticField(key)) {
        if (descriptor == "J")
            f.push(Value::fromLong(0));
        else if (descriptor == "F")
            f.push(Value::fromFloat(0.0f));
        else if (descriptor == "D")
            f.push(Value::fromDouble(0.0));
        else if (descriptor[0] == 'L' || descriptor[0] == '[')
            f.push(Value::null());
        else
            f.push(Value::fromInt(0));
        return;
    }
    f.push(loader_.getStaticField(key));
}

void Interpreter::op_putstatic() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    const cp_info& ref = f.cls->constant_pool[cp_index];
    std::string class_name = classNameFromConstantPool(*f.cls, ref.container.Fieldref.class_index);
    const cp_info& nat     = f.cls->constant_pool[ref.container.Fieldref.name_and_type_index];
    std::string field_name = utf8FromConstantPool(*f.cls, nat.container.NameAndType.name_index);

    Value val = f.pop();
    loader_.setStaticField(class_name + "::" + field_name, val);
}

void Interpreter::op_invokevirtual() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    const cp_info& ref  = f.cls->constant_pool[cp_index];
    std::string class_name  = classNameFromConstantPool(*f.cls, ref.container.Methodref.class_index);
    const cp_info& nat      = f.cls->constant_pool[ref.container.Methodref.name_and_type_index];
    std::string method_name = utf8FromConstantPool(*f.cls, nat.container.NameAndType.name_index);
    std::string descriptor  = utf8FromConstantPool(*f.cls, nat.container.NameAndType.descriptor_index);

    if (class_name == "java/io/PrintStream" &&
        (method_name == "println" || method_name == "print")) {
        simulatePrint(method_name, descriptor);
        return;
    }

    invokeInstanceMethod(loader_, frame_stack_, heap_, class_name, method_name, descriptor, true);
}

void Interpreter::simulatePrint(const std::string& method_name, const std::string& descriptor) {
    Frame& f = currentFrame();
    bool newline = (method_name == "println");

    if (descriptor == "()V") {
        f.pop();
        if (newline) std::cout << "\n";
        return;
    }

    if (descriptor == "(I)V" || descriptor == "(B)V" || descriptor == "(S)V") {
        int32_t val = f.pop().data.i;
        f.pop();
        std::cout << val;
    } else if (descriptor == "(J)V") {
        int64_t val = f.pop().data.l;
        f.pop();
        std::cout << val;
    } else if (descriptor == "(F)V") {
        float val = f.pop().data.f;
        f.pop();
        std::cout << val;
    } else if (descriptor == "(D)V") {
        double val = f.pop().data.d;
        f.pop();
        std::cout << val;
    } else if (descriptor == "(Z)V") {
        int32_t val = f.pop().data.i;
        f.pop();
        std::cout << (val ? "true" : "false");
    } else if (descriptor == "(C)V") {
        int32_t val = f.pop().data.i;
        f.pop();
        std::cout << static_cast<char>(val);
    } else if (descriptor == "(Ljava/lang/String;)V") {
        int32_t cp_index = f.pop().data.ref;
        f.pop();
        const cp_info& s = f.cls->constant_pool[cp_index];
        std::cout << utf8FromConstantPool(*f.cls, s.container.String.string_index);
    } else {
        throw std::runtime_error("simulatePrint: descritor nao suportado: " + descriptor);
    }

    if (newline) std::cout << "\n";
}

void Interpreter::op_new() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    std::string class_name = classNameFromConstantPool(*f.cls, cp_index);
    const class_info& cls  = loader_.load(class_name);

    int32_t ref = heap_.allocateObject(&cls, class_name);
    f.push(Value::fromRef(ref));
}

void Interpreter::op_newarray() {
    Frame& f = currentFrame();
    u1 atype  = fetchU1();
    int32_t length = f.pop().data.i;
    int32_t ref = heap_.allocateArray(atype, length);
    f.push(Value::fromRef(ref));
}

void Interpreter::op_anewarray() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();
    std::string element_class = classNameFromConstantPool(*f.cls, cp_index);
    int32_t length = f.pop().data.i;
    int32_t ref = heap_.allocateRefArray(element_class, length);
    f.push(Value::fromRef(ref));
}

// arraylength — desempilha uma referência de array e empilha seu comprimento.
void Interpreter::op_arraylength() {
    Frame& f = currentFrame();
    int32_t ref = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    f.push(Value::fromInt(heap_.arrayLength(ref)));
}

void Interpreter::op_iaload() {
    Frame& f = currentFrame();
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    f.push(Value::fromInt(heap_.getElement(ref, index).data.i));
}

void Interpreter::op_iastore() {
    Frame& f = currentFrame();
    int32_t value = f.pop().data.i;
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    heap_.setElement(ref, index, Value::fromInt(value));
}

void Interpreter::op_laload() {
    Frame& f = currentFrame();
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    f.push(Value::fromLong(heap_.getElement(ref, index).data.l));
}

void Interpreter::op_lastore() {
    Frame& f = currentFrame();
    int64_t value = f.pop().data.l;
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    heap_.setElement(ref, index, Value::fromLong(value));
}

void Interpreter::op_faload() {
    Frame& f = currentFrame();
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    f.push(Value::fromFloat(heap_.getElement(ref, index).data.f));
}

void Interpreter::op_fastore() {
    Frame& f = currentFrame();
    float   value = f.pop().data.f;
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    heap_.setElement(ref, index, Value::fromFloat(value));
}

void Interpreter::op_daload() {
    Frame& f = currentFrame();
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    f.push(Value::fromDouble(heap_.getElement(ref, index).data.d));
}

void Interpreter::op_dastore() {
    Frame& f = currentFrame();
    double  value = f.pop().data.d;
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    heap_.setElement(ref, index, Value::fromDouble(value));
}

void Interpreter::op_aaload() {
    Frame& f = currentFrame();
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    f.push(heap_.getElement(ref, index));
}

void Interpreter::op_aastore() {
    Frame& f = currentFrame();
    Value   value = f.pop();
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    heap_.setElement(ref, index, value);
}

void Interpreter::op_baload() {
    Frame& f = currentFrame();
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    f.push(Value::fromInt(heap_.getElement(ref, index).data.i));
}

void Interpreter::op_bastore() {
    Frame& f = currentFrame();
    int32_t value = f.pop().data.i;
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    heap_.setElement(ref, index, Value::fromInt(static_cast<int8_t>(value)));
}

void Interpreter::op_caload() {
    Frame& f = currentFrame();
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    f.push(Value::fromInt(heap_.getElement(ref, index).data.i));
}

void Interpreter::op_castore() {
    Frame& f = currentFrame();
    int32_t value = f.pop().data.i;
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    heap_.setElement(ref, index, Value::fromInt(static_cast<uint16_t>(value)));
}

void Interpreter::op_saload() {
    Frame& f = currentFrame();
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    f.push(Value::fromInt(heap_.getElement(ref, index).data.i));
}

void Interpreter::op_sastore() {
    Frame& f = currentFrame();
    int32_t value = f.pop().data.i;
    int32_t index = f.pop().data.i;
    int32_t ref   = f.pop().data.ref;
    if (heap_.isNull(ref))
        throw std::runtime_error("NullPointerException");
    heap_.setElement(ref, index, Value::fromInt(static_cast<int16_t>(value)));
}

namespace {
bool isSubtypeOf(ClassLoader& loader, const std::string& sub, const std::string& target) {
    if (sub == target || target == "java/lang/Object")
        return true;

    std::string current = sub;
    while (true) {
        const class_info* cls = nullptr;
        try {
            cls = &loader.load(current);
        } catch (...) {
            return false;
        }

        if (cls->super_class == 0)
            return false;

        std::string super = classNameFromConstantPool(*cls, cls->super_class);
        if (super == target)
            return true;
        if (super == "java/lang/Object")
            return false;
        current = super;
    }
}
} // namespace

void Interpreter::op_athrow() {
    Frame& f = currentFrame();
    int32_t ref = f.pop().data.ref;
    if (heap_.isNull(ref)) {
        int32_t npe = heap_.allocateObject(nullptr, "java/lang/NullPointerException");
        throw JavaException{ npe, "java/lang/NullPointerException" };
    }
    throw JavaException{ ref, heap_.object(ref).class_name };
}

void Interpreter::op_checkcast() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    int32_t ref = f.top().data.ref; // peek: o valor permanece na pilha
    if (heap_.isNull(ref))
        return;

    std::string target = classNameFromConstantPool(*f.cls, cp_index);
    std::string actual = heap_.object(ref).class_name;
    if (!isSubtypeOf(loader_, actual, target))
        throw std::runtime_error("ClassCastException: " + actual +
                                 " nao pode ser convertido para " + target);
}

void Interpreter::op_instanceof() {
    Frame& f = currentFrame();
    u2 cp_index = fetchU2();

    int32_t ref = f.pop().data.ref;
    if (heap_.isNull(ref)) {
        f.push(Value::fromInt(0));
        return;
    }

    std::string target = classNameFromConstantPool(*f.cls, cp_index);
    std::string actual = heap_.object(ref).class_name;
    f.push(Value::fromInt(isSubtypeOf(loader_, actual, target) ? 1 : 0));
}
