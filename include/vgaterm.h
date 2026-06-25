/* vgaterm.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef VGATERM_H
#define VGATERM_H
#include <kernel_common.h>

#define TERMINAL_N_ROWS (25)
#define TERMINAL_N_COLS (80)

enum terminal_color {
    terminal_black,
    terminal_blue,
    terminal_green,
    terminal_cyan,
    terminal_red,
    terminal_magenta,
    terminal_brown,
    terminal_light_grey,
    terminal_fg_dark_grey,       // fg only
    terminal_fg_bright_blue,     // fg only
    terminal_fg_bright_green,    // fg only
    terminal_fg_bright_cyan,     // fg only
    terminal_fg_bright_red,      // fg only
    terminal_fg_bright_magenta,  // fg only
    terminal_fg_yellow,          // fg only
    terminal_fg_white            // fg only
};

#define TERMINAL_CHARACTER_EMPTY (uint16_t)(0x20)

void terminal_initialize(void);

void terminal_set_attribute(enum terminal_color fg,
                            enum terminal_color bg);  // bg | blink

void terminal_putchar(char c);

void terminal_writestring(const char *s) KERNEL_NONNULL;

#endif  // VGATERM_H
