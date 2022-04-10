#include <poplar/Vertex.hpp>
/*
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

class InitVertex: public poplar::Vertex {
    public:
    poplar::InOut<poplar::Vector<char>> printBuf;

    bool compute() {
        char* poplar_stack_bottom;
        asm volatile("mov %[poplar_stack_bottom], $m11" : [poplar_stack_bottom] "+r" (poplar_stack_bottom) ::);
        poppy_init(&printBuf[0], poplar_stack_bottom);
        return true;
    }
};



class RTVertex: public poplar::Vertex {
    public:
    poplar::InOut<poplar::Vector<char>> printBuf;

    bool compute() {
        poppy_do_str("print('Persistent state!')\n", 0);
        return true;
    }
};


