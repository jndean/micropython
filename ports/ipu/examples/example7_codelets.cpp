#include <poplar/Vertex.hpp>

#include "example7.hpp"

/*  Provided by preprocessor
#include "poplar/StackSizeDefs.hpp" 
#define RECURSIVE_FUNCTION_SIZE (5 * 1024)
extern "C" void poppy_deinit(void);
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_init");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_deinit");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_add_memory_as_array");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_do_str");
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_set_stdout");
extern "C" void poppy_init(char *stdout_memory, char *poplar_stack_bottom);
extern "C" void poppy_add_memory_as_array(const char* name, void* data, size_t num_elts, char dtype);
extern "C" void poppy_do_str(const char *src, int is_single_line);
extern "C" void poppy_set_stdout(char* _stdout);
*/
DEF_STACK_USAGE(0, "poppy_set_stdin");
extern "C" void poppy_set_stdin(char* _stdin);
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "pyexec_event_repl_init");
extern "C" void pyexec_event_repl_init();
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "pyexec_event_repl_process_char");
extern "C" int pyexec_event_repl_process_char(int c);


class TerminalInit: public poplar::Vertex {
    public:
    // Global tensors
    poplar::InOut<poplar::Vector<char>> printBuf;
    poplar::InOut<bool> changeSubprocess;
    poplar::InOut<int> actionFlag;
    poplar::InOut<int> vfsPos;
    poplar::InOut<poplar::Vector<char>> vfsDataBlock;

    // Local tensors
    poplar::InOut<poplar::Vector<char>> terminalInBuf;

    bool compute() {
        char* poplar_stack_bottom;
        asm volatile(
            "mov %[poplar_stack_bottom], $m11" 
            : [poplar_stack_bottom] "+r" (poplar_stack_bottom) ::
        );
        poppy_set_stdout(&printBuf[0], printBuf.size());
        poppy_init(poplar_stack_bottom);
        poppy_add_memory_as_array("changeSubprocess", &*changeSubprocess, 1, 'b');
        poppy_add_memory_as_array("actionFlag", &*actionFlag, 1, 'b');
        poppy_add_memory_as_array("vfsPos", &*vfsPos, 1, 'b');
        poppy_add_memory_as_array("vfsDataBlock", &vfsDataBlock[0], vfsDataBlock.size(), 'b');
        poppy_add_memory_as_array("terminalInBuf", &terminalInBuf[0], terminalInBuf.size(), 'b');
        poppy_do_str(R"(

print("Not initialising VFS yet")

)", 0);

        *changeSubprocess = false;
        *actionFlag = flag_terminal;
        *vfsPos = -1;

        return true;
    }
};



class TerminalBody: public poplar::Vertex {
    public:
    // Global tensors
    poplar::InOut<poplar::Vector<char>> printBuf;
    poplar::InOut<bool> changeSubprocess;
    poplar::InOut<int> actionFlag;
    poplar::InOut<int> vfsPos;
    poplar::InOut<poplar::Vector<char>> vfsDataBlock;
    
    // Local tensors
    poplar::InOut<poplar::Vector<char>> terminalInBuf;

    bool compute() {
        poppy_set_stdout(&printBuf[0], printBuf.size());
        poppy_do_str("print('Body')\n", 0);

        *changeSubprocess = true;
        *actionFlag = flag_stop;
        return true;
    }
};


class VfsReadWrite: public poplar::Vertex {
    public:
    poplar::InOut<poplar::Vector<char>> disk;
    poplar::InOut<int> actionFlag;
    poplar::InOut<int> vfsPos;
    poplar::InOut<poplar::Vector<char>> vfsDataBlock;
    
    bool compute() {

        if (*vfsAction == msg_vfs_read) 
        {
            *vfsDataBlock = disk[*vfsPos];
        } 
        else if (*vfsAction == msg_vfs_write) 
        {
            disk[*vfsPos] = *vfsDataBlock;
        }
        return true;
    }
};