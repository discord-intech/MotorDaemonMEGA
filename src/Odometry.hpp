//
// Created by discord on 26/09/16.
//

#ifndef MOTORDAEMON_ODOMETRY_HPP
#define MOTORDAEMON_ODOMETRY_HPP

#include <Arduino.h>

#define CHAN_AL 18
#define CHAN_BL 19
#define CHAN_AR 20
#define CHAN_BR 21

#define MM_PER_TICK (1.4211)
#define TICKS_TO_RAD (0.00189)

class Odometry {


private:

    static long leftTicks;
    static long rightTicks;
    static int valueAL;
    static int valueBL;
    static int valueAR;
    static int valueBR;

    static void onTickChanALeft(void);
    static void onTickChanBLeft(void);
    static void onTickChanARight(void);
    static void onTickChanBRight(void);

public:
    Odometry();
    long getLeftValue();
    long getRightValue();

};



#endif //MOTORDAEMON_ODOMETRY_HPP
