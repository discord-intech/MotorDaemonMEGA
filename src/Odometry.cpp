//
// Created by discord on 26/09/16.
//

#include "Odometry.hpp"

#define AL 0
#define BL 1
#define AR 2
#define BR 3

long Odometry::leftTicks;
long Odometry::rightTicks;
int Odometry::valueAL;
int Odometry::valueBL;
int Odometry::valueAR;
int Odometry::valueBR;


int inline fast_atoi( const char * str )
{
    int val = 0;
    while( *str ) {
        val = val*10 + (*str++ - '0');
    }
    return val;
}

Odometry::Odometry()
{
    //TODO interrupt

    Odometry::leftTicks = 0;
    Odometry::rightTicks = 0;
    Odometry::valueAL = 0;
    Odometry::valueBL = 0;
    Odometry::valueAR = 0;
    Odometry::valueBR = 0;

}

void Odometry::mainWorker(char chanAL, char chanBL, char chanAR, char chanBR)
{
    /*int fdAL = open( (std::string("/sys/class/gpio/gpio")+std::to_string(chanAL)+std::string("/value")).c_str(), O_RDONLY );
    int fdBL = open( (std::string("/sys/class/gpio/gpio")+std::to_string(chanBL)+std::string("/value")).c_str(), O_RDONLY );
    int fdAR = open( (std::string("/sys/class/gpio/gpio")+std::to_string(chanAR)+std::string("/value")).c_str(), O_RDONLY );
    int fdBR = open( (std::string("/sys/class/gpio/gpio")+std::to_string(chanBR)+std::string("/value")).c_str(), O_RDONLY );

    struct pollfd pfd[4];

    pfd[AL].fd = fdAL;
    pfd[AL].events = POLLPRI;
    pfd[AL].revents = 0;

    pfd[BL].fd = fdBL;
    pfd[BL].events = POLLPRI;
    pfd[BL].revents = 0;

    pfd[AR].fd = fdAR;
    pfd[AR].events = POLLPRI;
    pfd[AR].revents = 0;

    pfd[BR].fd = fdBR;
    pfd[BR].events = POLLPRI;
    pfd[BR].revents = 0;

    while (true)
    {
        poll(pfd, 4, -1);

        if (pfd[AL].revents != 0) {
            get_lead(fdAL, AL);
            onTickChanALeft();
        }
        if (pfd[BL].revents != 0) {
            get_lead(fdBL, BL);
            onTickChanBLeft();
        }
        if (pfd[AR].revents != 0) {
            get_lead(fdAR, AR);
            onTickChanARight();
        }
        if (pfd[BR].revents != 0) {
            get_lead(fdBR, BR);
            onTickChanBRight();
        }

        //usleep(100);
        timespec t, r;
        t.tv_sec=0;
        t.tv_nsec = 50000;
        nanosleep(&t, &r);

    }*/
}

long Odometry::getLeftValue() {
    return Odometry::leftTicks;
}

long Odometry::getRightValue() {
    return Odometry::rightTicks;
}

void Odometry::onTickChanALeft(void)
{
    if(valueAL == valueBL)
    {
        ++leftTicks;
    }
}

void Odometry::onTickChanBLeft(void)
{
    if(valueAL == valueBL)
    {
        --leftTicks;
    }
}

void Odometry::onTickChanARight(void)
{
    if(valueAR == valueBR)
    {
        --rightTicks;
    }
}

void Odometry::onTickChanBRight(void)
{
    if(valueAR == valueBR)
    {
        ++rightTicks;
    }
}

void Odometry::get_lead(int& fd, char chan) //chan : 0=AL, 1=BL, 2=AR, 3=BR
{
   /* lseek(fd, 0, 0);

    char buffer[4];
    ssize_t size = read(fd, buffer, sizeof(buffer));

    if(chan == AL)
    {
        if (size != -1) {
            buffer[size] = '\0';
            valueAL = fast_atoi(buffer);
        }
        else {
            valueAL = -1;
        }
    } else if(chan == BL) {
        if (size != -1) {
            buffer[size] = '\0';
            valueBL = fast_atoi(buffer);
        }
        else {
            valueBL = -1;
        }
    } else if(chan == AR) {
        if (size != -1) {
            buffer[size] = '\0';
            valueAR = fast_atoi(buffer);
        }
        else {
            valueAR = -1;
        }
    } else if(chan == BR) {
        if (size != -1) {
            buffer[size] = '\0';
            valueBR = fast_atoi(buffer);
        }
        else {
            valueBR = -1;
        }
    }
*/
}

