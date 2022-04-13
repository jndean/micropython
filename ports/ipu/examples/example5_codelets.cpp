#include <poplar/Vertex.hpp>
/*  Provided by preprocessor
#include "poplar/StackSizeDefs.hpp" 
#define RECURSIVE_FUNCTION_SIZE (5 * 1024)
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_init");
extern "C" void poppy_deinit(void);
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_deinit");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_add_memory_as_array");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_do_str");
extern "C" void poppy_init(char *stdout_memory, char *poplar_stack_bottom);
extern "C" void poppy_add_memory_as_array(const char* name, void* data, size_t num_elts, char dtype);
extern "C" void poppy_do_str(const char *src, int is_single_line);
*/
DEF_STACK_USAGE(0, "poppy_set_stdin");
extern "C" void poppy_set_stdin(char* _stdin);
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_set_stdout");
extern "C" void poppy_set_stdout(char* _stdout);
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "pyexec_event_repl_init");
extern "C" void pyexec_event_repl_init();
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "pyexec_event_repl_process_char");
extern "C" int pyexec_event_repl_process_char(int c);

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
    // poplar::InOut<poplar::Vector<char>> diskImg;
    poplar::InOut<poplar::Vector<char>> printBuf;
    poplar::InOut<poplar::Vector<char>> inBuf;
    poplar::InOut<bool> doneFlag;

    bool compute(unsigned tid) {
        char* poplar_stack_bottom;
        
        if (tid != 5) return true;
        asm volatile(
            "mov %[poplar_stack_bottom], $m11" 
            : [poplar_stack_bottom] "+r" (poplar_stack_bottom) ::
        );
        poppy_init(&printBuf[0], poplar_stack_bottom);
        poppy_add_memory_as_array("inbuf", &inBuf[0], inBuf.size(), 'b');

        *doneFlag = false;
   
        return true;
    }
};

bool checkpoint_is_live = false;

class RTVertex: public poplar::MultiVertex {
    public:
    // poplar::InOut<poplar::Vector<char>> diskImg;
    poplar::InOut<poplar::Vector<char>> printBuf;
    poplar::InOut<poplar::Vector<char>> inBuf;
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

        poppy_set_stdout(&printBuf[0]);
        
        poppy_do_str(R"(
for x in range(5):
    print('inBuf:', inbuf)
    exchange
)", 0);


        *doneFlag = true;
        return true;
    }
};


