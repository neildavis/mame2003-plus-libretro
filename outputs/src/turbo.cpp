#include <string.h>

#include "output-def.h"

void turbo_init() {

}

void turbo_stop() {

}

void turbo_output(const char *output_name, int value) {
    int n;
    if (0 == strcmp(output_name, OUTPUTS_INIT_NAME)) {
        turbo_init();
        return;
    }
    if (0 == strcmp(output_name, OUTPUTS_STOP_NAME)) {
        turbo_stop();
        return;
    }
}

