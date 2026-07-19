/* keyboard.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <keyboard.h>
#include <stdbool.h>
#include <stdint.h>
#include <vgaterm.h>

// #define

#define KEY_LEFT_SHIFT_MAKE  (0x2A)
#define KEY_LEFT_SHIFT_BREAK (0xAA)

#define KEY_RIGHT_SHIFT_MAKE  (0x36)
#define KEY_RIGHT_SHIFT_BREAK (0xB6)

#define KEY_CAPS_MAKE  (0x3A)
#define KEY_CAPS_BREAK (0xBA)

static bool shift_held = false;
static bool caps_active = false;

// clang-format off
static const char lower[128] = {
/*        +0    +1    +2    +3    +4    +5    +6    +7    +8    +9    +A    +B    +C    +D    +E    +F   */
/* 0x00 */  0,    0,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=', '\b', '\t',
/* 0x10 */ 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']', '\n',   0,  'a',  's',
/* 0x20 */ 'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',  '`',   0,  '\\', 'z',  'x',  'c',  'v',
/* 0x30 */ 'b',  'n',  'm',  ',',  '.',  '/',   0,   '*',   0,   ' ',   0,    0,    0,    0,    0,    0,
/* 0x40 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x50 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x60 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x70 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};

static const char upper[128] = {
/*        +0    +1    +2    +3    +4    +5    +6    +7    +8    +9    +A    +B    +C    +D    +E    +F   */
/* 0x00 */  0,    0,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',  '+', '\b', '\t',
/* 0x10 */ 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}', '\n',   0,  'A',  'S',
/* 0x20 */ 'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':', '"',  '~',   0,   '|', 'Z',  'X',  'C',  'V',
/* 0x30 */ 'B',  'N',  'M',  '<',  '>',  '?',   0,   '*',   0,   ' ',   0,    0,    0,    0,    0,    0,
/* 0x40 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x50 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x60 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x70 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
// clang-format on

static void toggle_caps(void) {
    caps_active ^= true;
}

void keyboard_handler(uint8_t scancode) {
    uint8_t make = scancode & 0x7F;
    bool is_break = scancode & 0x80;
    uint8_t break_ = make | 0x80;
    bool use_upper = false;
    char ch = 0;

    if (is_break) {  // break codes
        switch (break_) {
            case KEY_RIGHT_SHIFT_BREAK:;
            case KEY_LEFT_SHIFT_BREAK:
                shift_held = false;
                break;
        }

    } else {  // make codes
        switch (make) {
            case KEY_RIGHT_SHIFT_MAKE:;
            case KEY_LEFT_SHIFT_MAKE:
                shift_held = true;
                break;
            case KEY_CAPS_MAKE:
                toggle_caps();
                break;
            default: {
                use_upper = shift_held ^ (caps_active && lower[make] >= 'a' &&
                                          lower[make] <= 'z');
                ch = use_upper ? upper[make] : lower[make];
            }; break;
        }
    }
    if (ch != 0) {
        terminal_putchar(ch);
    }
}
