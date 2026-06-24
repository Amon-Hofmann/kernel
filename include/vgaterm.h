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
    terminal_fg_dark_grey,
    terminal_fg_bright_green,
    terminal_fg_bright_cyan,
    terminal_fg_bright_red,
    terminal_fg_bright_magenta,
    terminal_fg_yellow,
    terminal_fg_white
};

#define TERMINAL_CHARACTER_EMPTY (uint16_t)(0x20)

void terminal_initialize(void);

void terminal_set_attribute(char fg, char bg);  // bg | blink

void terminal_putchar(char c);

void terminal_writestring(const char *s) _NONNULL;

#endif  // VGATERM_H
