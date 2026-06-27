/* pit.c
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#include <io.h>
#include <kernel_common.h>
#include <pit.h>

#define PIT_BASE_FREQ_HZ (1193182)
#define PIT_CMD_PORT     (0x43)
#define PIT_CH0_PORT     (0x40)
#define PIT_MODE_RATE    (0x36)

static volatile uint32_t pit_ticks = 0;
static uint32_t pit_hz = 0;

void pit_init(uint32_t hz) {
    uint32_t divisor = PIT_BASE_FREQ_HZ / hz;
    pit_hz = hz;
    port_io_write_byte(PIT_MODE_RATE, PIT_CMD_PORT);
    port_io_write_byte(divisor & 0xFF, PIT_CH0_PORT);         // low byte
    port_io_write_byte((divisor >> 8) & 0xFF, PIT_CH0_PORT);  // high byte
}

uint32_t pit_get_ticks(void) {
    return pit_ticks;
}

void ksleep_ms(uint32_t ms) {
    uint32_t ticks = (pit_hz * ms) / 1000;
    uint32_t start = pit_ticks;
    while ((pit_ticks - start) < ticks) {
        __asm__ volatile("hlt");
    }
}

void pit_inc_ticks(void) {
    pit_ticks++;
}
