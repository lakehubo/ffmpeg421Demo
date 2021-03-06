//
// Created by lake on 2020/3/13.
//
/**
 * 同步时钟
 */
#ifndef FFMPEG421DEMO_L_CLOCK_H
#define FFMPEG421DEMO_L_CLOCK_H

#define AV_NOSYNC_THRESHOLD 10.0
/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
typedef struct Clock {
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

double get_clock(Clock *c);
void set_clock_at(Clock *c, double pts, int serial, double time);
void set_clock(Clock *c, double pts, int serial);
void set_clock_speed(Clock *c, double speed);
void init_clock(Clock *c, int *queue_serial);
void sync_clock_to_slave(Clock *c, Clock *slave);

#endif //FFMPEG421DEMO_L_CLOCK_H
