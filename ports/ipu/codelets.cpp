
#include <poplar/Vertex.hpp> 
#include <stdint.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "shared/runtime/pyexec.h"



#if MICROPY_ENABLE_COMPILER
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}
#endif

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[20480];
#endif

int volatile sideeffect = 1;
int test_func(int count){
    sideeffect += 1;
    if (count == 0) return sideeffect;
    int val = test_func(count - 1);
    return val + sideeffect;
}


class pyvertex: public poplar::Vertex {
    public:
    poplar::InOut<poplar::Vector<char>> stack;

// int main(int argc, char **argv) {
    bool compute() {
        int stack_dummy;
        stack_top = (char *)&stack_dummy;

        #if MICROPY_ENABLE_GC
        gc_init(heap, heap + sizeof(heap));
        #endif

        #if MICROPY_ENABLE_PYSTACK
        static mp_obj_t pystack[64 * 1024];
        mp_pystack_init(pystack, &pystack[MP_ARRAY_SIZE(pystack)]);
        #endif

        mp_init();
        do_str("print('\\nhello world!')\nprint(list(x+1 for x in range(10)), end='\\n')", MP_PARSE_FILE_INPUT);//MP_PARSE_SINGLE_INPUT);
        
        stack[1] = test_func(stack[0]);

        // #if MICROPY_ENABLE_COMPILER
        // #if MICROPY_REPL_EVENT_DRIVEN
        // pyexec_event_repl_init();
        // for (;;) {
        //     int c = mp_hal_stdin_rx_chr();
        //     if (pyexec_event_repl_process_char(c)) {
        //         break;
        //     }
        // }
        // #else
        // pyexec_friendly_repl();
        // #endif
        // do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT);
        // do_str("for i in range(10):\r\n  print(i)", MP_PARSE_FILE_INPUT);
        // #else
        // pyexec_frozen_module("frozentest.py");
        // #endif
        mp_deinit();
        // return 0;
        return true;
    }
};

#if MICROPY_ENABLE_GC
void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info();
}
#endif

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

// mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
//     return mp_const_none;
// }
// MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);


void nlr_jump_fail(void *val) {
    while (1) {
        ;
    }
}

void NORETURN __fatal_error(const char *msg) {
    asm volatile("trap 0\n" :::);
    __builtin_unreachable();
}


#ifndef NDEBUG
#include <stdio.h>
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
