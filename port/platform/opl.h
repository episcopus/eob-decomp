/* opl.h: the AdLib's chip, a Yamaha YM3812 (OPL2), as ymfm emulates it (playbook 80, decision 7). */
#ifndef OPL_H
#define OPL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the chip's own sample rate, on the AdLib's 3.579545 MHz clock: 3579545 / 72 */
#define OPL_RATE 49716

void opl_write(uint8_t reg, uint8_t value);
void opl_render(int16_t *samples, unsigned count);

#ifdef __cplusplus
}
#endif

#endif
