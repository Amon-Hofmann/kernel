/* pit.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef PIT_H
#define PIT_H

#include <kernel_common.h>
#include <stdint.h>

void pit_init(uint32_t hz);
void ksleep_ms(uint32_t ms);
uint32_t pit_get_ticks(void);
void pit_inc_ticks(void);

#endif  // PIT_H
