/*
 * Tin Bot Controller Software
 */

#include <stdlib.h>
#include <time.h>

#include "hal.h"
#include "rhr.h"

#include "tinbot.h"

/* BEGIN CHOICE BLOCK
 * This is the actual implementation that will be executed.
 * Choose any of:
 * - mode/tinbot-rhr.c (works in matlab, works in reality)
 * - mode/tinbot-vicdir.c (not implemented)
 * - mode/tinbot-alone.c (not implemented)
 * - mode/tinbot-full.c (not implemented)
 */

#include "mode/tinbot-vicdir.c"

/* END CHOICE BLOCK */

static double prox_to_cm(double measured) {
    return measured; /* FIXME */
}

void update_proximity(TinBot* tinbot, double proximity[8]) {
    unsigned int number;
    for (number = 0; number < 8; number++) {
        /*if (proximity[number] > 50) {
            hal_set_led(number, 1);
        } else {
            hal_set_led(number, 0);
        }*/
        tinbot->sens.proximity[number] = prox_to_cm(proximity[number]);
    }
}

void update_ir(TinBot* tinbot, int ir[6]) {
    unsigned int i;
    for (i = 0; i < 6; ++i) {
        tinbot->sens.ir[i] = ir[i];
    }
}

void update_lps(TinBot* tinbot, double x, double y, double direction) {
    /* FIXME */
}
