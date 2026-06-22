/* pic.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef PIC_H
#define PIC_H
#include <stdint.h>

void pic_remap(void);
void pic_send_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);
void pic_irq_mask_all(void);

#endif  // PIC_H
