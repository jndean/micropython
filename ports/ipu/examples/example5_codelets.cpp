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
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "pyexec_event_repl_init");
extern "C" void pyexec_event_repl_init();
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "pyexec_event_repl_process_char");
extern "C" int pyexec_event_repl_process_char(int c);
DEF_STACK_USAGE(RECURSIVE_FUNCTION_SIZE, "poppy_set_stdout");
extern "C" void poppy_set_stdout(char* _stdout);

class InitVertex: public poplar::Vertex {
    public:
    poplar::InOut<poplar::Vector<char>> diskImg;
    poplar::InOut<poplar::Vector<char>> printBuf;
    poplar::InOut<poplar::Vector<char>> inBuf;
    poplar::InOut<bool> doneFlag;

    bool compute() {
        char* poplar_stack_bottom;
        asm volatile(
            "mov %[poplar_stack_bottom], $m11" 
            : [poplar_stack_bottom] "+r" (poplar_stack_bottom) ::
        );
        poppy_init(&printBuf[0], poplar_stack_bottom);
        poppy_add_memory_as_array("__diskimg", &diskImg[0], diskImg.size(), 'b');
        poppy_do_str(R"(
import uos as os

class TensorBlockDevice:
    def __init__(self, block_size):
        self.block_size = block_size

    def readblocks(self, block_num, buf, offset=0):
        addr = block_num * self.block_size + offset
        poppyyield
        for i in range(len(buf)):
            buf[i] = __diskimg[addr + i]

    def writeblocks(self, block_num, buf, offset=None):
        if offset is None:
            # do erase, then write
            for i in range(len(buf) // self.block_size):
                self.ioctl(6, block_num + i)
            offset = 0
        addr = block_num * self.block_size + offset
        for i in range(len(buf)):
            __diskimg[addr + i] = buf[i]

    def ioctl(self, op, arg):
        if op == 4: # block count
            return len(__diskimg) // self.block_size
        if op == 5: # block size
            return self.block_size
        if op == 6: # block erase
            return 0

__disk_device = TensorBlockDevice(block_size=512)
#os.VfsLfs2.mkfs(__bdev)
os.mount(__disk_device, '/')

)", 0);

        *doneFlag = false;
        pyexec_event_repl_init();
    
        return true;
    }
};



class RTVertex: public poplar::Vertex {
    public:
    poplar::InOut<poplar::Vector<char>> diskImg;
    poplar::InOut<poplar::Vector<char>> printBuf;
    poplar::InOut<poplar::Vector<char>> inBuf;
    poplar::InOut<bool> doneFlag;

    bool compute() {
        poppy_set_stdout(&printBuf[0]);
        char c = inBuf[0];
        *doneFlag = (c == '\0') || pyexec_event_repl_process_char(c);
        return true;
    }
};


