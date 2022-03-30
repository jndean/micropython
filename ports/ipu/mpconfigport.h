#include <stdint.h>

// options to control how MicroPython is built

// Use the minimal starting configuration (disables all optional features).
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.
#define MICROPY_ENABLE_COMPILER     (1)

// #define MICROPY_QSTR_EXTRA_POOL           mp_qstr_frozen_const_pool
#define MICROPY_ENABLE_GC                 (1)
#define MICROPY_HELPER_REPL               (0)
#define MICROPY_MODULE_FROZEN_MPY         (0)
#define MICROPY_ENABLE_EXTERNAL_IMPORT    (0)


// Desperate mitigations for IPU
#define MICROPY_USE_INTERNAL_ERRNO        (1)
#define MICROPY_ENABLE_PYSTACK            (1)
#define MICROPY_NLR_SETJMP                (1)

// Adding more interesting interpreter feaures
#define MICROPY_PY_ARRAY                   (1)
#define MICROPY_PY_BUILTINS_SLICE          (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN      (1)
#define MICROPY_PY_BUILTINS_FILTER         (1)
#define MICROPY_PY_BUILTINS_REVERSED       (1)
#define MICROPY_PY_BUILTINS_SET            (1)
#define MICROPY_PY_BUILTINS_ENUMERATE      (1)
#define MICROPY_FLOAT_IMPL                 (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_PY_COLLECTIONS             (1)
#define MICROPY_PY_COLLECTIONS_DEQUE       (1)
#define MICROPY_PY_UHEAPQ                  (0)

#define MICROPY_ALLOC_PATH_MAX            (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT    (16)

// type definitions for the specific machine

typedef intptr_t mp_int_t; // must be pointer size
typedef uintptr_t mp_uint_t; // must be pointer size
typedef long mp_off_t;

// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) },


#define MICROPY_HW_BOARD_NAME "ipu"
#define MICROPY_HW_MCU_NAME "Colossus"

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[8];
