/* vgaterm.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vgaterm.h>

#define VGA_MEM_ADDRESS          (0xB8000)
#define VGA_TERM_ROW_N_BYTES     (TERMINAL_N_COLS * 2)
#define VGA_TERM_24_ROWS_N_BYTES (VGA_TERM_ROW_N_BYTES * (TERMINAL_N_ROWS - 1))

volatile uint16_t *vga =
    (volatile uint16_t *)VGA_MEM_ADDRESS;  // VGA MEM Address

static uint16_t s_vga_term_attribute = 0;
static uint8_t s_vga_term_cursor_column = 0;
static uint8_t s_vga_term_cursor_row = 0;

void terminal_initialize(void) {
    s_vga_term_cursor_row = 0;
    s_vga_term_cursor_column = 0;
    // terminal_set_attribute(TERMINAL_FG_LIGHT_GREY, TERMINAL_BG_BLACK);
    terminal_set_attribute(TERMINAL_FG_LIGHT_GREY,
                           TERMINAL_BG_RED);  // muattion exercise part A

    for (uint16_t cursor = 0; cursor < TERMINAL_N_ROWS * TERMINAL_N_COLS;
         cursor++) {
        terminal_putchar(' ');
    }
    s_vga_term_cursor_row = 0;
    s_vga_term_cursor_column = 0;
}

void terminal_set_attribute(const char fg, const char bg) {
    s_vga_term_attribute = (uint16_t)bg << 4 | (uint16_t)fg;
}

static inline void terminal_inc_cursor(void) {
    if (++s_vga_term_cursor_column >= TERMINAL_N_COLS) {
        s_vga_term_cursor_column = 0;

        if (s_vga_term_cursor_row >= TERMINAL_N_ROWS - 1) {
            memmove((void *)vga, (void *)(vga + TERMINAL_N_COLS),
                    VGA_TERM_24_ROWS_N_BYTES);

            for (uint8_t index = 0; index < TERMINAL_N_COLS; index++) {
                vga[TERMINAL_N_COLS * (TERMINAL_N_ROWS - 1) + index] =
                    s_vga_term_attribute << 8 | 0x20;
            }
            // s_vga_term_cursor_row = 0; Before Mutation exercise part B
        } else {
            s_vga_term_cursor_row++;
        }
    }
}

void terminal_putchar(const char c) {
    vga[TERMINAL_N_COLS * s_vga_term_cursor_row + s_vga_term_cursor_column] =
        (uint16_t)s_vga_term_attribute << 8 | c;
    terminal_inc_cursor();
}

void terminal_writestring(const char *s) {
    const char *head = s;
    while (*head != '\0') {
        terminal_putchar(*head++);
    }
}
