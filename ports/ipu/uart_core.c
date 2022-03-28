
#ifndef __IPU__
#include <unistd.h>
#else
// #include <ipuunistd.h>
#endif
#include "py/mpconfig.h"

/*
 * Core UART functions to implement for a port
 */


// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    #if MICROPY_MIN_USE_STDOUT
    int r = read(STDIN_FILENO, &c, 1);
    (void)r;
    #else
        c = '\n';
    #endif
    return c;
}

// Send string of given length
char * stdout_head = 0;
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    #if MICROPY_MIN_USE_STDOUT
    int r = write(STDOUT_FILENO, str, len);
    (void)r;
    #else
    for (int i = 0; i < len; ++i) {
        (*stdout_head++) = str[i];
    }
    #endif
}
