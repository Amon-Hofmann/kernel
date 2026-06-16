/* serial.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <serial.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SERIAL_COM1_BASE ((uint16_t)0x3F8)

#define SERIAL_THR_OFFSET (0x00)

#define SERIAL_DLL_MAX_RATE ((uint16_t)0x01)
#define SERIAL_DLL_OFFSET   (0x00)

#define SERIAL_DLH_MAX_RATE ((uint16_t)0x00)
#define SERIAL_DLH_OFFSET   (0x01)

#define SERIAL_FCR_FIFO_MODE ((uint16_t)0xC7)  // a bit unspecific
#define SERIAL_FCR_OFFSET    (0x02)

#define SERIAL_LCR_8N1_DLAB_1 ((uint16_t)0x83)
#define SERIAL_LCR_8N1_DLAB_0 ((uint16_t)0x03)
#define SERIAL_LCR_OFFSET     (0x03)

#define SERIAL_MCR_NO_INTERRUPTS ((uint16_t)0x03)
#define SERIAL_MCR_OFFSET        (0x04)

#define SERIAL_IER_NO_INTERRUPTS ((uint16_t)0x00)
#define SERIAL_IER_OFFSET        (0x0)

#define SERIAL_LSR_OFFSET     (0x05)
#define SERIAL_LSR_THRE_SHIFT (0x05)

static inline void write_byte(uint8_t value, uint16_t port) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t read_byte(uint16_t port) {
    uint8_t volatile ret = 0;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void) {
    // 1. Set DLAB=1 (LCR bit 7) to access the divisor SERIAL: 8N1
    write_byte(SERIAL_LCR_8N1_DLAB_1, SERIAL_COM1_BASE + SERIAL_LCR_OFFSET);

    // 2. Write baud rate divisor low byte to offset +0
    write_byte(SERIAL_DLL_MAX_RATE, SERIAL_COM1_BASE + SERIAL_DLL_OFFSET);

    // 3. Write baud rate divisor high byte to offset +1
    write_byte(SERIAL_DLH_MAX_RATE, SERIAL_COM1_BASE + SERIAL_DLH_OFFSET);

    // 4. Clear DLAB, set 8N1: write `0x03` to LCR (offset +3)
    write_byte(SERIAL_LCR_8N1_DLAB_0, SERIAL_COM1_BASE + SERIAL_LCR_OFFSET);

    // 5. Enable and reset FIFOs: write `0xC7` to FCR (offset +2)
    write_byte(SERIAL_FCR_FIFO_MODE, SERIAL_COM1_BASE + SERIAL_FCR_OFFSET);

    // 6. Enable DTR and RTS: write `0x03` to MCR (offset +4)
    write_byte(SERIAL_MCR_NO_INTERRUPTS, SERIAL_COM1_BASE + SERIAL_MCR_OFFSET);

    // 7. Disable all interrupts: write `0x00` to IER (offset +1)
    write_byte(SERIAL_IER_NO_INTERRUPTS, SERIAL_COM1_BASE + SERIAL_IER_OFFSET);
}

void serial_putchar(char c) {
    uint8_t volatile val = 0;
    while (!(val & (1 << SERIAL_LSR_THRE_SHIFT))) {  // while THRE bit is 0
        val = read_byte(SERIAL_COM1_BASE + SERIAL_LSR_OFFSET);
    }
    write_byte(c, SERIAL_COM1_BASE + SERIAL_THR_OFFSET);
}

void serial_writestring(const char *s) {
    while (*s) {
        serial_putchar(*s);
        s++;
    }
}

static void serial_write_32_hex(uint32_t n, bool upper) {
    serial_writestring("0x");
    char const *where;
    if (upper) {
        where = "0123456789ABCDEF";
    } else {
        where = "0123456789abcdef";
    }
    for (size_t i = 0; i < 8; i++) {
        serial_putchar(where[(n & 0xF0000000) >> 7 * 4]);
        n = n << 4;
    }
}

void serial_printf(const char *format, ...) {
    bool hit = false;
    va_list args;
    va_start(args, format);

    while (*format) {
        if (!hit && *format == '%') {
            // first '%' found
            hit = true;

        } else if (hit && *format == '%') {
            // second '%' in a row, ignore
            hit = false;
            serial_putchar('%');
        }

        else if (hit) {
            // format specifier
            switch (*format) {
                case 'x':
                    serial_write_32_hex(va_arg(args, uint32_t),
                                        false);  // interpret as hex
                    break;
                case 'X':
                    serial_write_32_hex(va_arg(args, uint32_t),
                                        true);  // interpret as hex
                    break;
                case 's':
                    serial_writestring(
                        va_arg(args, const char *));  // interpret as string
                    break;
            }
            hit = false;
        } else {
            serial_putchar(*format);
        }
        format++;
    }
    va_end(args);
}
