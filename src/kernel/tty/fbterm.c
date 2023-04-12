#include <kernel/tty.h>
#include <kernel/video.h>
#include <kernel/console.h>

static ssize_t fb_term_output(size_t sz, uint8_t *buf);

static ssize_t fb_term_output(size_t sz, __unused uint8_t *buf) {
    return sz;
}

void fb_term_install(void) {
    console_set_output(fb_term_output);
}
