/* opl.cpp: the AdLib's chip for sdl_host.c, on ymfm (BSD 3-clause, `make ymfm`). The only C++ of the port. */
#include "ymfm_opl.h"

#include "opl.h"

namespace {
class Host : public ymfm::ymfm_interface {};    /* the chip's timers are the machine's (ports.c): nothing to hook */
Host host;
ymfm::ym3812 chip(host);
}

extern "C" void opl_write(uint8_t reg, uint8_t value)
{
    chip.write_address(reg);
    chip.write_data(value);
}

extern "C" void opl_render(int16_t *samples, unsigned count)
{
    ymfm::ym3812::output_data out;
    for (unsigned i = 0; i < count; i++) {
        chip.generate(&out);
        samples[i] = (int16_t)out.data[0];
    }
}
