#include <hal/hal.h>
#include <assert.h>
#include <pi.h>
#include "traffic-cop-eyes.h"

enum TCE_STATES {
    TCE_inactive, /* not enough information about the victim's location */
    TCE_waitdetect, /* wait until next measurement takes place */
    TCE_waitoff, /* waited long enough, next measurement as early as possible, i.e. when next signal is detected */
    TCE_done /* we know the victim's position */
};

static const double MIN_DIST = 12; /* arbitrary, TODO */

static double ray_dist(double x, double y, double phi) {
    double ray_orth_x = -sin(phi);
    double ray_orth_y =  cos(phi);
    double dist = fabs(x * ray_orth_x + y * ray_orth_y); /* scalar product of [xy] and ray_orth */
    return dist;
}

void tce_reset(TCEState* tce){
    tce->locals.state = TCE_inactive;
    tce->need_angle = 0;
    tce->locals.last_x = 0;
    tce->locals.last_y = -2 * MIN_DIST;
    tce->locals.last_phi = 0;
}

void tce_step(TCEInputs* inputs, TCEState* tce, Sensors* sens){
    if(inputs->found_victim_xy) {
        tce->locals.state = TCE_done;
        tce->need_angle = 0;
    }

    switch(tce->locals.state) {
        case TCE_inactive:
            {
                const double dist =
                    ray_dist(sens->current.x - tce->locals.last_x,
                             sens->current.y - tce->locals.last_y,
                             tce->locals.last_phi);
                if(inputs->ir_stable && dist >= MIN_DIST) {
                    tce->locals.state = TCE_waitdetect;
                    tce->need_angle = 1;
                }
            }
            break;
        case TCE_waitdetect:
            if (inputs->found_victim_phi || inputs->phi_give_up) {
                tce->locals.state = TCE_waitoff;
                tce->need_angle = 0;
                if (inputs->found_victim_phi) {
                    tce->locals.last_x = sens->current.x;
                    tce->locals.last_y = sens->current.y;
                    tce->locals.last_phi = inputs->ray_phi;
                }
            }
            break;
        case TCE_waitoff:
            if (!inputs->found_victim_phi && !inputs->phi_give_up) {
                tce->locals.state = TCE_inactive;
            }
            break;
        case TCE_done:
            break;
        default:
            /* Uhh */
            hal_print("TrafficCop Eyes illegal state?!  Resetting ...");
            assert(0);
            tce_reset(tce);
            break;
    }
}
