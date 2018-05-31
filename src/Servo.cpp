//
// Created by discord on 30/09/16.
//

#include "Servo.hpp"


Servo::Servo(float lowB, float lowA, float upB, float upA, char id) : lowerBound(lowB), upperBound(upB),
                                                                      lowerAngle(lowA), upperAngle(upA), id(id)
{}

void Servo::initAx(void)
{
    Dynamixel.begin(9600);
    Dynamixel.setSRL(id, 0);
    Dynamixel.setMaxTorque(id, 1023);
    Dynamixel.moveSpeed(id, 512, 1023);
}

void Servo::setAngle(double angle)
{

    if(angle < lowerAngle) angle = lowerAngle;
    if(angle > upperAngle) angle = upperAngle;

    int16_t value = (int16_t) ((upperBound - lowerBound) / (upperAngle - lowerAngle) * (angle + ABS(lowerAngle)) + lowerBound);
    Dynamixel.moveSpeed(id, value, 1023);


}

void Servo::setLed(bool b)
{
    Dynamixel.ledStatus(id, b);
}
