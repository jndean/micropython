#include <poplar/Vertex.hpp>

/*  Provided by preprocessor
#include "poplar/StackSizeDefs.hpp" 
#define RECURSIVE_FUNCTION_SIZE (5 * 1024)
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_init");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_set_stdout");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_deinit");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_add_memory_as_array");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_do_str");
extern "C" void poppy_init(char *stdout_memory, char *poplar_stack_bottom);
extern "C" void poppy_deinit(void);
extern "C" void poppy_add_memory_as_array(const char* name, void* data, size_t num_elts, char dtype);
extern "C" void poppy_do_str(const char *src, int is_single_line);
extern "C" void poppy_set_stdout(char* _stdout);
*/

typedef struct jmp_buf_t {
    unsigned int MRF[5];
    float ARF[2];
} jmp_buf[1];
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "setjmp");
extern "C" int __attribute__((noinline)) setjmp(jmp_buf env);
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "longjmp");
extern "C" void __attribute__((noreturn)) longjmp(jmp_buf env, int val);
extern "C" jmp_buf poppy_exit_env;
extern "C" jmp_buf poppy_checkpoint_env;


class InitVertex: public poplar::MultiVertex {
    public:
    poplar::InOut<poplar::Vector<char>> printBuf;
    poplar::InOut<poplar::Vector<char>> msgBuf;
    poplar::InOut<bool> doneFlag;

    bool compute(unsigned tid) {
        if (tid != 5) return true;
        
        char* poplar_stack_bottom;
        asm volatile(
            "mov %[poplar_stack_bottom], $m11" 
            : [poplar_stack_bottom] "+r" (poplar_stack_bottom) ::
        );
        poppy_set_stdout(&printBuf[0], printBuf.size());
        poppy_init(poplar_stack_bottom);
        poppy_add_memory_as_array("msgBuf", &msgBuf[0], msgBuf.size(), 'b');

        *doneFlag = false;
        return true;
    }
};


bool checkpoint_is_live = false;

class RTVertex: public poplar::MultiVertex {
    public:
    poplar::InOut<poplar::Vector<char>> printBuf;
    poplar::InOut<poplar::Vector<char>> msgBuf;
    poplar::InOut<bool> doneFlag;

    bool compute(unsigned tid) {
        if (tid != 5) return true;

        if (checkpoint_is_live) {
            checkpoint_is_live = false;
            longjmp(poppy_checkpoint_env, 1);
        }
        checkpoint_is_live = setjmp(poppy_exit_env);
        if (checkpoint_is_live) {
            return true;
        }
        poppy_set_stdout(&printBuf[0], printBuf.size());
        poppy_do_str(R"(


def main():
    message = ""
    while message != "STOP":
        longyield
        message = "".join(chr(byte) for byte in msgBuf)
        print("Received:", message)

if __name__ == "__main__":
    main()


)", 0);

        *doneFlag = true;
        return true;
    }
};

