
#include <poplar/Vertex.hpp> 
#include "poplar/StackSizeDefs.hpp" 

#define RECURSIVE_FUNCTION_SIZE (5 * 1024)

DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_deinit");
extern "C" void poppy_deinit(void);

DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_init");
extern "C" void poppy_init(char *stdout_memory, char *poplar_stack_bottom);

DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_add_memory_as_array");
extern "C" void poppy_add_memory_as_array(const char* name, void* data, size_t num_elts);

DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_do_str");
extern "C" void poppy_do_str(const char *src, int is_single_line);




class pyvertex: public poplar::Vertex {
    public:
    poplar::InOut<poplar::Vector<char>> stdout_tensor;
    poplar::InOut<poplar::Vector<int>> variable1;

    bool compute() {
        char* poplar_stack_bottom;
        asm volatile(
            "mov %[poplar_stack_bottom], $m11"
            : [poplar_stack_bottom] "+r" (poplar_stack_bottom) : : 
        );

        poppy_init(&stdout_tensor[0], poplar_stack_bottom);

        poppy_add_memory_as_array("variable1", &variable1[0], variable1.size());


        poppy_do_str("import uarray\n", 1);
        poppy_do_str("print(uarray, variable1)\n", 0);
        poppy_do_str("variable1[1] = 1701\n", 0);

        poppy_deinit();

        return true;
    }
};

