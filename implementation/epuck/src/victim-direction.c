#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "hal/hal.h"
#include "pi.h"
#include "sensors.h"
#include "state-machine-common.h"
#include "victim-direction.h"
#include "t2t-parse.h"

#define LOG_TRANSITIONS_VICDIR

/* Invariants:
 * state==VD_done \iff precisely one output flag is set
 * state!=VD_done \iff none of the output flags are set */
enum {
    VD_off,
    VD_running,
    VD_done
};

/* We know that the IR sensors should at *least*
 * have a sensibility of this many degrees. */
static const double VD_MIN_ON = 9.0 / 360.0;

void vd_reset(VDState* vd){
    vd->locals.state = VD_off;
    vd->victim_found = 0;
    vd->victim_phi = 0;
    vd->give_up = 0;
    vd->locals.counter_total = 1;
    vd->locals.counter_on = 0;
    vd->locals.weighted_sum = 0;
    vd->locals.gap_phi = -1;
}

typedef char check_num_ir_even[(NUM_IR % 2 == 0) ? 1 : -1];

static void entry_start(VDState* vd, Sensors* sens) {
    int i, have_id;
    vd->locals.state = VD_running;
    have_id = 0;
    for(i = 0; i < NUM_IR; ++i) {
        if (sens->ir[i]) {
            int sensor_id = (i + NUM_IR / 2) % NUM_IR;
            have_id = 1;
            vd->locals.gap_phi = ir_sensor_angle[sensor_id];
            break;
        }
    }
    if (have_id == 0) {
        vd->locals.state = VD_done;
        vd->give_up = 1;
    } else {
        smc_rot_left();
        vd->locals.counter_on = 1;
        vd->locals.weighted_sum = M_PI;
        vd->locals.time_begin = hal_get_time();
    }
}

static void compute_result(VDState* vd, Sensors* sens) {
    double eff_opening;

    if (vd->locals.counter_total < 100 * NUM_IR) {
        /* Wat. */
        vd->give_up = 1;
        return;
    }

    eff_opening = vd->locals.counter_on * 1.0 / (NUM_IR * vd->locals.counter_total);
    if (eff_opening < VD_MIN_ON) {
        /* There is no spoon.  And no VICTOR. */
        vd->give_up = 1;
        return;
    }

    vd->victim_phi = sens->current.phi + vd->locals.gap_phi
        + (2 * M_PI + vd->locals.weighted_sum / vd ->locals.counter_on);
    vd->victim_phi = fmod(vd->victim_phi + 2 * M_PI, 2 * M_PI);

    #ifdef LOG_TRANSITIONS_VICDIR
    {
        char buf[7 /* "VD:phi=" */ + 2 /* sign & magnitude */
                 + 1 /* '.' */ + 3 /* decimals */ + 1 /* NUL */];
        if (-9 < vd->victim_phi && vd->victim_phi < 99) {
            sprintf(buf, "VD:phi=%.3f", vd->victim_phi);
        } else {
            sprintf(buf, "VD:phi=inf");
        }
        hal_print(buf);
    }
    #endif
    send_found_phi(sens->current.x, sens->current.y, vd->victim_phi);
    vd->victim_found = 1;
}

void vd_step(VDState* vd, Sensors* sens){
    switch(vd->locals.state) {
        case VD_off:
            entry_start(vd, sens);
            break;
        case VD_running:
            {
                const double rot_angle = (hal_get_time() - vd->locals.time_begin)
                                     * SMC_ROT_PER_SEC / 1000.0;
                int i;
                vd->locals.counter_total += 1;
                for (i = 0; i < NUM_IR; ++i) {
                    if (sens->ir[i]) {
                        vd->locals.counter_on += 1;
                        vd->locals.weighted_sum +=
                            fmod(rot_angle + ir_sensor_angle[i] - vd->locals.gap_phi + 2 * M_PI, 2 * M_PI);
                    }
                }
                if (rot_angle >= 2 * M_PI) {
                    vd->locals.state = VD_done;
                    smc_halt();
                    compute_result(vd, sens);
                }
            }
            break;
        case VD_done:
            break;
        default:
            hal_print("Invalid state in victim direction. VICTOR, where are you??");
            assert(0);
            vd_reset(vd);
            break;
    }
}
