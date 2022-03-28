
#include <poplar/Vertex.hpp> 
#include "poplar/StackSizeDefs.hpp" 
#include <stdint.h>
#include <string.h>

// #include "py/compile.h"
// #include "py/runtime.h"
// #include "py/repl.h"
// #include "py/gc.h"
// #include "py/mperrno.h"
// #include "shared/runtime/pyexec.h"



#include "py/ipusetjmp.h"


#define MICROPY_ENABLE_GC  (1)
#define MICROPY_ENABLE_PYSTACK (1)
#define MICROPY_ENABLE_GC      (1)
#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_NLR_SETJMP (1)
// MICROPY_PY_FSTRINGS not sure what this should be, think (0)


DEF_STACK_USAGE(1024*5, "mp_init");
extern "C" void mp_init(void);
DEF_STACK_USAGE(1024*5, "mp_deinit");
extern "C" void mp_deinit(void);

extern "C" void gc_init(void *start, void *end);

typedef uint64_t mp_obj_t;
#define MP_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
extern "C" void mp_pystack_init(void *start, void *end);

typedef enum {
    MP_PARSE_SINGLE_INPUT,
    MP_PARSE_FILE_INPUT,
    MP_PARSE_EVAL_INPUT,
} mp_parse_input_kind_t;

typedef struct _nlr_buf_t nlr_buf_t;
struct _nlr_buf_t {
    nlr_buf_t *prev;
    void *ret_val;
    #if MICROPY_NLR_SETJMP
    jmp_buf jmpbuf;
    #else
    void *regs[MICROPY_NLR_NUM_REGS];
    #endif
    #if MICROPY_ENABLE_PYSTACK
    void *pystack;
    #endif
};

extern "C" unsigned int nlr_push_tail(nlr_buf_t *nlr);
#if MICROPY_NLR_SETJMP
#define nlr_push(buf) (nlr_push_tail(buf), setjmp((buf)->jmpbuf))
#else
extern "C" unsigned int nlr_push(nlr_buf_t *);
#endif

typedef enum _mp_token_kind_t {
    MP_TOKEN_END,

    MP_TOKEN_INVALID,
    MP_TOKEN_DEDENT_MISMATCH,
    MP_TOKEN_LONELY_STRING_OPEN,
    #if MICROPY_PY_FSTRINGS
    MP_TOKEN_MALFORMED_FSTRING,
    MP_TOKEN_FSTRING_RAW,
    #endif

    MP_TOKEN_NEWLINE,
    MP_TOKEN_INDENT,
    MP_TOKEN_DEDENT,

    MP_TOKEN_NAME,
    MP_TOKEN_INTEGER,
    MP_TOKEN_FLOAT_OR_IMAG,
    MP_TOKEN_STRING,
    MP_TOKEN_BYTES,

    MP_TOKEN_ELLIPSIS,

    MP_TOKEN_KW_FALSE,
    MP_TOKEN_KW_NONE,
    MP_TOKEN_KW_TRUE,
    MP_TOKEN_KW___DEBUG__,
    MP_TOKEN_KW_AND,
    MP_TOKEN_KW_AS,
    MP_TOKEN_KW_ASSERT,
    #if MICROPY_PY_ASYNC_AWAIT
    MP_TOKEN_KW_ASYNC,
    MP_TOKEN_KW_AWAIT,
    #endif
    MP_TOKEN_KW_BREAK,
    MP_TOKEN_KW_CLASS,
    MP_TOKEN_KW_CONTINUE,
    MP_TOKEN_KW_DEF,
    MP_TOKEN_KW_DEL,
    MP_TOKEN_KW_ELIF,
    MP_TOKEN_KW_ELSE,
    MP_TOKEN_KW_EXCEPT,
    MP_TOKEN_KW_FINALLY,
    MP_TOKEN_KW_FOR,
    MP_TOKEN_KW_FROM,
    MP_TOKEN_KW_GLOBAL,
    MP_TOKEN_KW_IF,
    MP_TOKEN_KW_IMPORT,
    MP_TOKEN_KW_IN,
    MP_TOKEN_KW_IS,
    MP_TOKEN_KW_LAMBDA,
    MP_TOKEN_KW_NONLOCAL,
    MP_TOKEN_KW_NOT,
    MP_TOKEN_KW_OR,
    MP_TOKEN_KW_PASS,
    MP_TOKEN_KW_RAISE,
    MP_TOKEN_KW_RETURN,
    MP_TOKEN_KW_TRY,
    MP_TOKEN_KW_WHILE,
    MP_TOKEN_KW_WITH,
    MP_TOKEN_KW_YIELD,

    MP_TOKEN_OP_ASSIGN,
    MP_TOKEN_OP_TILDE,

    // Order of these 6 matches corresponding mp_binary_op_t operator
    MP_TOKEN_OP_LESS,
    MP_TOKEN_OP_MORE,
    MP_TOKEN_OP_DBL_EQUAL,
    MP_TOKEN_OP_LESS_EQUAL,
    MP_TOKEN_OP_MORE_EQUAL,
    MP_TOKEN_OP_NOT_EQUAL,

    // Order of these 13 matches corresponding mp_binary_op_t operator
    MP_TOKEN_OP_PIPE,
    MP_TOKEN_OP_CARET,
    MP_TOKEN_OP_AMPERSAND,
    MP_TOKEN_OP_DBL_LESS,
    MP_TOKEN_OP_DBL_MORE,
    MP_TOKEN_OP_PLUS,
    MP_TOKEN_OP_MINUS,
    MP_TOKEN_OP_STAR,
    MP_TOKEN_OP_AT,
    MP_TOKEN_OP_DBL_SLASH,
    MP_TOKEN_OP_SLASH,
    MP_TOKEN_OP_PERCENT,
    MP_TOKEN_OP_DBL_STAR,

    // Order of these 13 matches corresponding mp_binary_op_t operator
    MP_TOKEN_DEL_PIPE_EQUAL,
    MP_TOKEN_DEL_CARET_EQUAL,
    MP_TOKEN_DEL_AMPERSAND_EQUAL,
    MP_TOKEN_DEL_DBL_LESS_EQUAL,
    MP_TOKEN_DEL_DBL_MORE_EQUAL,
    MP_TOKEN_DEL_PLUS_EQUAL,
    MP_TOKEN_DEL_MINUS_EQUAL,
    MP_TOKEN_DEL_STAR_EQUAL,
    MP_TOKEN_DEL_AT_EQUAL,
    MP_TOKEN_DEL_DBL_SLASH_EQUAL,
    MP_TOKEN_DEL_SLASH_EQUAL,
    MP_TOKEN_DEL_PERCENT_EQUAL,
    MP_TOKEN_DEL_DBL_STAR_EQUAL,

    MP_TOKEN_DEL_PAREN_OPEN,
    MP_TOKEN_DEL_PAREN_CLOSE,
    MP_TOKEN_DEL_BRACKET_OPEN,
    MP_TOKEN_DEL_BRACKET_CLOSE,
    MP_TOKEN_DEL_BRACE_OPEN,
    MP_TOKEN_DEL_BRACE_CLOSE,
    MP_TOKEN_DEL_COMMA,
    MP_TOKEN_DEL_COLON,
    MP_TOKEN_DEL_PERIOD,
    MP_TOKEN_DEL_SEMICOLON,
    MP_TOKEN_DEL_EQUAL,
    MP_TOKEN_DEL_MINUS_MORE,
} mp_token_kind_t;

typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
typedef unsigned int uint;
typedef unsigned char byte;

typedef struct _mp_reader_t {
    void *data;
    mp_uint_t (*readbyte)(void *data);
    void (*close)(void *data);
} mp_reader_t;

#if MICROPY_PY_BUILTINS_STR_UNICODE
typedef uint32_t unichar;
#else
typedef uint unichar;
#endif

typedef size_t qstr;

typedef struct _vstr_t {
    size_t alloc;
    size_t len;
    char *buf;
    bool fixed_buf : 1;
} vstr_t;


typedef struct _mp_lexer_t {
    qstr source_name;           // name of source
    mp_reader_t reader;         // stream source
    unichar chr0, chr1, chr2;   // current cached characters from source
    #if MICROPY_PY_FSTRINGS
    unichar chr0_saved, chr1_saved, chr2_saved; // current cached characters from alt source
    #endif

    size_t line;                // current source line
    size_t column;              // current source column

    mp_int_t emit_dent;             // non-zero when there are INDENT/DEDENT tokens to emit
    mp_int_t nested_bracket_level;  // >0 when there are nested brackets over multiple lines

    size_t alloc_indent_level;
    size_t num_indent_level;
    uint16_t *indent_level;

    size_t tok_line;            // token source line
    size_t tok_column;          // token source column
    mp_token_kind_t tok_kind;   // token kind
    vstr_t vstr;                // token data
    #if MICROPY_PY_FSTRINGS
    vstr_t fstring_args;        // extracted arguments to pass to .format()
    size_t fstring_args_idx;    // how many bytes of fstring_args have been read
    #endif
} mp_lexer_t;

extern "C" mp_lexer_t *mp_lexer_new_from_str_len(qstr src_name, const char *str, size_t len, size_t free_len);

typedef struct _mp_parse_chunk_t {
    size_t alloc;
    union {
        size_t used;
        struct _mp_parse_chunk_t *next;
    } union_;
    byte data[];
} mp_parse_chunk_t;

typedef uintptr_t mp_parse_node_t;
typedef struct _mp_parse_t {
    mp_parse_node_t root;
    struct _mp_parse_chunk_t *chunk;
} mp_parse_tree_t;

extern "C" mp_parse_tree_t mp_parse(mp_lexer_t *lex, mp_parse_input_kind_t input_kind);
extern "C" mp_obj_t mp_compile(mp_parse_tree_t *parse_tree, qstr source_file, bool is_repl);
extern "C" mp_obj_t mp_call_function_0(mp_obj_t fun);
extern "C" void nlr_pop(void);

typedef void (*mp_print_strn_t)(void *data, const char *str, size_t len);
typedef struct _mp_print_t {
    void *data;
    mp_print_strn_t print_strn;
} mp_print_t;
extern "C" void mp_obj_print_exception(const mp_print_t *print, mp_obj_t exc);
extern "C" const mp_print_t mp_plat_print;

// QDEF(MP_QSTR__lt_stdin_gt_, 227, 7, "<stdin>")
#define MP_QSTR__lt_stdin_gt_ 227


#if MICROPY_ENABLE_COMPILER
DEF_STACK_USAGE(1024*5, "do_str");
extern "C" void do_str(const char *src, mp_parse_input_kind_t input_kind) {
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


extern "C" char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[2048];
#endif

extern "C" char* stdout_head;


class pyvertex: public poplar::Vertex {
    public:
    poplar::InOut<poplar::Vector<char>> stdout_tensor;

    bool compute() {
        // int stack_dummy;
        // stack_top = (char *)&stack_dummy;
        stdout_head = &stdout_tensor[0];
        asm volatile(
            "mov %[stack_top], $m11"
            : [stack_top] "+r" (stack_top) : : 
        );

        #if MICROPY_ENABLE_GC
        gc_init(heap, heap + sizeof(heap));
        #endif

        #if MICROPY_ENABLE_PYSTACK
        static mp_obj_t pystack[10 * 1024];
        mp_pystack_init(pystack, &pystack[MP_ARRAY_SIZE(pystack)]);
        #endif

        mp_init();
        #if MICROPY_ENABLE_COMPILER
        do_str("print('\\nhello world!')\nprint(list(x+1 for x in range(10)), end='\\n')", MP_PARSE_FILE_INPUT);//MP_PARSE_SINGLE_INPUT);
        #endif

        // // #if MICROPY_ENABLE_COMPILER
        // // #if MICROPY_REPL_EVENT_DRIVEN
        // // pyexec_event_repl_init();
        // // for (;;) {
        // //     int c = mp_hal_stdin_rx_chr();
        // //     if (pyexec_event_repl_process_char(c)) {
        // //         break;
        // //     }
        // // }
        // // #else
        // // pyexec_friendly_repl();
        // // #endif
        // // do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT);
        // // do_str("for i in range(10):\r\n  print(i)", MP_PARSE_FILE_INPUT);
        // // #else
        // // pyexec_frozen_module("frozentest.py");
        // // #endif
        // mp_deinit();
        // return 0;

        return true;
    }
};

// #if MICROPY_ENABLE_GC
// extern "C" void gc_collect(void) {
//     // WARNING: This gc_collect implementation doesn't try to get root
//     // pointers from CPU registers, and thus may function incorrectly.
//     void *dummy;
//     gc_collect_start();
//     gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
//     gc_collect_end();
//     gc_dump_info();
// }
// #endif

// mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
//     mp_raise_OSError(MP_ENOENT);
// }

// mp_import_stat_t mp_import_stat(const char *path) {
//     return MP_IMPORT_STAT_NO_EXIST;
// }

// mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
//     return mp_const_none;
// }
// MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);


// void nlr_jump_fail(void *val) {
//     while (1) {
//         ;
//     }
// }

// void NORETURN __fatal_error(const char *msg) {
//     asm volatile("trap 0\n" :::);
//     __builtin_unreachable();
// }


// #ifndef NDEBUG
// #include <stdio.h>
// void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
//     printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
//     __fatal_error("Assertion failed");
// }
// #endif
