/* vgaterm.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef VGATERM_H
#define VGATERM_H

#define TERMINAL_N_ROWS (25)
#define TERMINAL_N_COLS (80)

#define TERMINAL_FG_BLACK          (0)
#define TERMINAL_FG_BLUE           (1)
#define TERMINAL_FG_GREEN          (2)
#define TERMINAL_FG_CYAN           (3)
#define TERMINAL_FG_RED            (4)
#define TERMINAL_FG_MAGENTA        (5)
#define TERMINAL_FG_BROWN          (6)
#define TERMINAL_FG_LIGHT_GREY     (7)
#define TERMINAL_FG_DARK_GREY      (8)
#define TERMINAL_FG_BRIGHT_BLUE    (9)
#define TERMINAL_FG_BRIGHT_GREEN   (10)
#define TERMINAL_FG_BRIGHT_CYAN    (11)
#define TERMINAL_FG_BRIGHT_RED     (12)
#define TERMINAL_FG_BRIGHT_MAGENTA (13)
#define TERMINAL_FG_YELLOW         (14)
#define TERMINAL_FG_WHITE          (15)

#define TERMINAL_BG_BLACK      (0)
#define TERMINAL_BG_BLUE       (1)
#define TERMINAL_BG_GREEN      (2)
#define TERMINAL_BG_CYAN       (3)
#define TERMINAL_BG_RED        (4)
#define TERMINAL_BG_MAGENTA    (5)
#define TERMINAL_BG_BROWN      (6)
#define TERMINAL_BG_LIGHT_GREY (7)

#define TERMINAL_CHARACTER_EMPTY (uint16_t)(0x20)

void terminal_initialize(void);

void terminal_set_attribute(const char fg, const char bg);  // bg | blink

void terminal_putchar(const char c);

void terminal_writestring(const char *s);

#endif  // VGATERM_H
