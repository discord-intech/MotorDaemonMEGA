//
// Created by discord on 26/09/16.
//

#ifndef MOTORDAEMON_MOTOR_HPP
#define MOTORDAEMON_MOTOR_HPP

#include "lib/safe_enum.hpp"

#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))
#define ABS(x) (((x) > 0) ? (x) : -(x))
#define SIN(x) (1-(((x)*(x)*(x))/6.0))
#define COS(x) (1-(((x)*(x))/4.0))
#define ARCTAN(x) ((x)-(((x)*(x)*(x))/3.0))
#define TAN(x) ((x)+(((x)*(x)*(x))/3.0))

#define PWM_TIME_PERIOD (1000.0*1000.0)  // nanosecondes

#define MINIMAL_PWM_PERC (settings.getFloat("MIN_PWM_PERC"))
#define MAXIMUM_PWM_PERC (settings.getFloat("MAX_PWM_PERC")) //Used to limit PWM output

#define INVERTED_PWM true
#define LEFT_MOTOR_PWM_PIN 2
#define RIGHT_MOTOR_PWM_PIN 3

#define LEFT_MOTOR_DIR_PIN_1 26
#define LEFT_MOTOR_DIR_PIN_2 28
#define RIGHT_MOTOR_DIR_PIN_1 30
#define RIGHT_MOTOR_DIR_PIN_2 32


    struct direction_def {
        enum type {
            BACKWARD, FORWARD
        };
    };

    typedef safe_enum<direction_def> Direction;

    class Motor {
    private:
       // BlackLib::BlackPWM pwm = BlackLib::BlackPWM(BlackLib::PWMDISABLE);
        char PWMpin;

        int directionPin;
        int directionPin2;

        bool inversed;

        Direction actualDirection;
        int actualDuty;
        void setDirection(Direction);

    public:
        Motor(char, int, int, bool);
        void initPWM(void);
        void run(int);
    };


    class LeftMotor : public Motor {
    public:
        LeftMotor();
    };

    class RightMotor : public Motor {
    public:
        RightMotor();
    };




#endif //MOTORDAEMON_MOTOR_HPP
