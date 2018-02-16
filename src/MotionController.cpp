//
// Created by discord on 26/09/16.
//

#include "MotionController.hpp"
#include <math.h>

bool MotionController::started;
long MotionController::startTime;
long MotionController::execTime;


unsigned long Millis(void)
{
    return millis();
}

unsigned long Micros(void)
{
    return micros();
}

double fastSin( double x )
{
    x = fmod(x + 3.14f, 3.14f * 2) - 3.14f; // restrict x so that -M_PI < x < M_PI
    const float B = 4.0f/3.14f;
    const float C = -4.0f/(3.14f*3.14f);

    double y = B * x + C * x * ABS(x);

    const float P = 0.225f;

    return P * (y * ABS(y) - y) + y;
}

MotionController::MotionController() :  rightMotor(), leftMotor(), direction(1650000, LOW_ANGLE, 2050000, HIGH_ANGLE), //FIXME bounds
rightSpeedPID(), leftSpeedPID(), translationPID(), curvePID(),
averageLeftSpeed(), averageRightSpeed(), odo()
{
    execTime = 0;
    startTime = 0;
    
    rightSpeedPID.setPointers(currentRightSpeed, rightPWM, rightSpeedSetpoint);
    leftSpeedPID.setPointers(currentLeftSpeed, leftPWM, leftSpeedSetpoint);
    translationPID.setPointers(currentDistance, translationSpeed, translationSetpoint);
    curvePID.setPointers(currentRadius, deltaRadius, curveSetpoint);

    leftSpeedPID.setOutputLimits(-255,255);
    rightSpeedPID.setOutputLimits(-255,255);
    curvePID.setOutputLimits((int32_t) (DIST_MOTOR_DIRECTION / tan((float)LOW_ANGLE)),
                             (int32_t) (DIST_MOTOR_DIRECTION / tan((float)HIGH_ANGLE)));

    leftSpeedPID.setEpsilon(20);
    rightSpeedPID.setEpsilon(20);

    servoMotor = 0;



    maxSpeed = 2000; // Vitesse maximum, des moteurs
    maxSpeedTranslation = 2000; // Consigne max envoy�e au PID
    maxAcceleration = 600;
    maxDecceleration = 600;
    leftCurveRatio = 1.0;
    rightCurveRatio = 1.0;

    // maxjerk = 1; // Valeur de jerk maxi(secousse d'acc�l�ration)

    toleranceTranslation = 40;
    toleranceSpeed = 50;
    toleranceSpeedEstablished = 50; // Doit �tre la plus petite possible, sans bloquer les trajectoires courbes 50
    delayToEstablish = 1000;


    toleranceDifferentielle = 500; // Pour les trajectoires "normales", v�rifie que les roues ne font pas nawak chacunes de leur cot�.

    translationPID.setTunings(0,0,0);
    leftSpeedPID.setTunings(0.2,0.0001,0);
    rightSpeedPID.setTunings(0.2,0.0001,0);
    curvePID.setTunings(0,0,0);

    distanceTest = 200;

    delayToStop = 100;

}

void MotionController::init()
{
    leftMotor.initPWM();
    rightMotor.initPWM();
    direction.initPWM();

    direction.setAngle(0);

    compute_direction_table();

    started = true;
}

void MotionController::mainWorker(MotionController *&asser)
{
    int count=0;
    long lastTime = Millis();


    while(started)
    {

        if(*(asser->stahp))
        {
            delay(1000 / FREQ_ASSERV);
            continue;
        }

        asser->control();
        count++;

        if(count % 5 == 0)
        {
            asser->manageStop();
        }

        if(count % 10 == 0)
        {
            asser->updatePosition();
        }

        if(count == 50000)
        {
            count = 0;
            //std::cout << "Frequency : " << 50000000./(double)(Millis() - lastTime) << " Hz" << std::endl;
           // asser->printTranslationError();
            lastTime = Millis();
        }

        execTime = Millis() - startTime;

        delay(static_cast<uint32_t>((1000 / FREQ_ASSERV) - execTime));
    }
}

void MotionController::control()
{
    static long time = Millis();

//    static long freq(0);

 //   static int counter(0);

    static long relativeDistanceOrigin(0);

    // Pour le calcul de la vitesse instantan�e :
    static long previousLeftTicks = 0;
    static long previousRightTicks = 0;

    // Pour le calcul de l'acc�l�ration intantan�e :
    static long previousLeftSpeedSetpoint = 0;
    static long previousRightSpeedSetpoint = 0;

    /*
    // Pour le calcul du jerk :
    static int32_t previousLeftAcceleration = 0;
    static int32_t previousRightAcceleration = 0;
    */

    if(sweeping)
    {
        sweepRadius += (sweepRadius >= 0) ? -80 : 80;
        if(ABS(sweepRadius) < 1000)
        {
            sweepRadius = (sweepRadius <= 0) ? -1000 : 1000;
        }
        *curveSetpoint = sweepRadius;
    }

    if(GOcounter > 0)
    {
        GOcounter++;
        if(GOcounter > GO_COUNTER_THRESHOLD)
        {
            GOcounter = 0;
            //std::cout << "Stopped for go timeout." << std::endl;
            stop();
        }
    }

    long rightTicks = odo.getRightValue();

    long leftTicks = odo.getLeftValue();

 /*   if(freq == 0)
    {
        *currentLeftSpeed = (leftTicks - previousLeftTicks)*FREQ_ASSERV; // (nb-de-tick-passés)*(freq_asserv) (ticks/sec)
        *currentRightSpeed = (rightTicks - previousRightTicks)*FREQ_ASSERV;
    }
    else
    {
        *currentLeftSpeed = (leftTicks - previousLeftTicks)*freq; // (nb-de-tick-passés)*(freq_asserv) (ticks/sec)
        *currentRightSpeed = (rightTicks - previousRightTicks)*freq;
    }*/

    long actualTime = Micros();

    if(actualTime == 0)
    {
      //  perror("Could not read time!\n");
    }

    *currentLeftSpeed = (long) ((leftTicks - previousLeftTicks) / ((actualTime-startTime) / 1000000.)); // (nb-de-tick-passés)*(freq_asserv) (ticks/sec)
    *currentRightSpeed = (long) ((rightTicks - previousRightTicks) / ((actualTime-startTime) / 1000000.));

    startTime = actualTime;

    previousLeftTicks = leftTicks;
    previousRightTicks = rightTicks;

    averageLeftSpeed.add(*currentLeftSpeed);
    averageRightSpeed.add(*currentRightSpeed);

    *currentLeftSpeed = averageLeftSpeed.value(); // On utilise pour l'asserv la valeur moyenne des dernieres current Speed
    *currentRightSpeed = averageRightSpeed.value(); // sinon le robot il fait nawak.

    if(ABS(*currentRightSpeed - *currentLeftSpeed) > 5)
    {
        *currentRadius = (long) ((*currentLeftSpeed * RAYON_COD_DROITE + *currentRightSpeed * RAYON_COD_GAUCHE)
                                         / (MM_PER_TICK * (*currentRightSpeed - *currentLeftSpeed)));
    }
    else
    {
        *currentRadius = INT32_MAX;
    }


    *currentDistance = (leftTicks + rightTicks) / 2;
    *currentAngle = fmod((*originAngle + TICKS_TO_RAD*(double)(rightTicks - leftTicks)) + 3.14f, 3.14f * 2) - 3.14f;


    /*if(!pointsToPass.empty() && moving)
    {
        if(lastWay != pointsToPass.front().way)
        {
            lastWay = pointsToPass.front().way;
            long dist = (long)((double)(*currentDistance - *translationSetpoint)*(double)MM_PER_TICK*(lastWay ? 1.0 : -1.0));
            stop();
            orderTranslation(dist);
        }

        if(ABS(*currentDistance - relativeDistanceOrigin)*MM_PER_TICK >= pointsToPass.front().relativeDistance)
        {
            *curveSetpoint = (long) pointsToPass.front().curvePoint;
            pointsToPass.pop();
            curvePID.resetErrors();
        }
    }
    else
    {
        relativeDistanceOrigin = *currentDistance;
    }*/

    if(controlled) translationPID.compute();

    if(controlled && leftCurveRatio != 0 && rightCurveRatio != 0 && *currentLeftSpeed > 20 && *currentRightSpeed > 20
       && ABS((double)*currentRightSpeed / (double)*currentLeftSpeed) - (rightCurveRatio / leftCurveRatio) > 0.01)
    {
        curvePID.compute();
    }
    else
    {
        *deltaRadius = 0;
    }

    if(ABS(*curveSetpoint + *deltaRadius) < MAX_RADIUS)
    {
        leftCurveRatio = ((double)ABS(*curveSetpoint + *deltaRadius)-(RAYON_COD_GAUCHE*((*curveSetpoint<0)?-1.0:1.0)))/((double)ABS(*curveSetpoint + *deltaRadius)+RAYON_COD_DROITE-RAYON_COD_GAUCHE);
        rightCurveRatio = ((double)ABS(*curveSetpoint + *deltaRadius)+(RAYON_COD_DROITE*((*curveSetpoint<0)?-1.0:1.0)))/((double)ABS(*curveSetpoint + *deltaRadius)+RAYON_COD_DROITE-RAYON_COD_GAUCHE);
    }
    else
    {
        leftCurveRatio = 1.0;
        rightCurveRatio = 1.0;
    }

    if(MAX(leftCurveRatio, rightCurveRatio) > 1.0)
    {
        float offset = (float) (1.0 - MAX(leftCurveRatio, rightCurveRatio));
        leftCurveRatio = MAX(leftCurveRatio+offset,0);
        rightCurveRatio = MAX(rightCurveRatio+offset,0);
    }

    if(leftCurveRatio<0)
        leftCurveRatio=0;
    if(rightCurveRatio<0)
        rightCurveRatio=0;


    // Limitation de la consigne de vitesse en translation
    if(*translationSpeed > maxSpeedTranslation)
        *translationSpeed = maxSpeedTranslation;
    else if(*translationSpeed < -maxSpeedTranslation)
        *translationSpeed = -maxSpeedTranslation;


    *leftSpeedSetpoint = (long) (*translationSpeed * leftCurveRatio);
    *rightSpeedSetpoint = (long) (*translationSpeed * rightCurveRatio);

    // Limitation de la vitesse
    if(*leftSpeedSetpoint > maxSpeed)
        *leftSpeedSetpoint = maxSpeed;
    else if(*leftSpeedSetpoint < -maxSpeed)
        *leftSpeedSetpoint = -maxSpeed;
    if(*rightSpeedSetpoint > maxSpeed)
        *rightSpeedSetpoint = maxSpeed;
    else if(*rightSpeedSetpoint < -maxSpeed)
        *rightSpeedSetpoint = -maxSpeed;


    // Limitation de l'accélération du moteur gauche (permet de règler la pente du trapèze de vitesse)
    if(*leftSpeedSetpoint - previousLeftSpeedSetpoint > maxAcceleration)
    {
        *leftSpeedSetpoint = (long) (previousLeftSpeedSetpoint + maxAcceleration * leftCurveRatio);
    }
    else if(*leftSpeedSetpoint - previousLeftSpeedSetpoint < -maxDecceleration)
    {
        *leftSpeedSetpoint = (long) (previousLeftSpeedSetpoint - maxDecceleration * leftCurveRatio);
    }

    // Limitation de l'acc�l�ration du moteur droit
    if(*rightSpeedSetpoint - previousRightSpeedSetpoint > maxAcceleration)
    {
        *rightSpeedSetpoint = (long) (previousRightSpeedSetpoint + maxAcceleration * rightCurveRatio);
    }
    else if(*rightSpeedSetpoint - previousRightSpeedSetpoint < -maxDecceleration)
    {
        *rightSpeedSetpoint = (long) (previousRightSpeedSetpoint - maxDecceleration * rightCurveRatio);
    }



    previousLeftSpeedSetpoint = *leftSpeedSetpoint;			// Mise à jour des consignes de vitesse
    previousRightSpeedSetpoint = *rightSpeedSetpoint;



    leftSpeedPID.compute();		// Actualise la valeur de 'leftPWM'
    rightSpeedPID.compute();	// Actualise la valeur de 'rightPWM'

    //std::cout << "calculation time : " << Millis() - time << std::endl;
   // time = Millis();

    leftMotor.run((int) *leftPWM);
    rightMotor.run((int) *rightPWM);

    long t = Millis();

    if(t-time >= DELTA_FREQ_REFRESH)
    {
        //freq = counter / (t - time);
        time = t;
        //counter = 0;
       // std::cout << "it's me : " << (long)translationPID.getPTR() << " : " <<(long)&currentDistance << " : " << currentDistance << " : " << translationSetpoint << " : " <<translationPID.getError() << std::endl;
      /*  std::cout << "it's me : " << *leftPWM << ";" << *leftSpeedSetpoint << ";" << averageLeftSpeed.value() << " : " << *rightPWM << ";" << *rightSpeedSetpoint << ";" << averageLeftSpeed.value()
                  << " : " << *currentDistance << ";" << *translationSetpoint << " : " << leftCurveRatio << ";" << rightCurveRatio
                  << " : " << *curveSetpoint << ";" << *deltaRadius << " : "
                  << ((*curveSetpoint + *deltaRadius)>0 ? 1.0 : -1.0) * ((ABS(*curveSetpoint + *deltaRadius) >= MAX_RADIUS) ? direction_table[MAX_RADIUS-1] : direction_table[ABS(*curveSetpoint + *deltaRadius)])
                  << std::endl;*/
    }
   // else counter++;

    //std::cout << "PWM time : " << Millis() - time << std::endl;

    if(servoMotor && ABS(*curveSetpoint + *deltaRadius) >= MAX_RADIUS)
    {
        direction.setAngle(((*curveSetpoint + *deltaRadius) > 0 ? 1.0 : -1.0)*direction_table[MAX_RADIUS-1]);
    }
    else if(servoMotor)
    {
        direction.setAngle(((*curveSetpoint + *deltaRadius) > 0 ? 1.0 : -1.0)*direction_table[ABS(*curveSetpoint + *deltaRadius)]);
    }

    //direction.setAngle( ((*curveSetpoint + *deltaRadius)>0 ? 1.0 : -1.0)
    //                    * (1.5707 - atan((float)ABS(*curveSetpoint + *deltaRadius) / (float)DIST_MOTOR_DIRECTION)));
    //direction.setAngle(0);
}

void MotionController::stop()
{

    //std::cout << "DEBUG : STOP" << std::endl;

    *stahp = true;

    leftMotor.run(0);
    rightMotor.run(0);

    delay(1);

    leftMotor.run(0);
    rightMotor.run(0);

    *currentDistance = (odo.getRightValue()+odo.getLeftValue())/2;
    *translationSetpoint = *currentDistance;
    *translationSpeed = 0;
    *leftSpeedSetpoint = 0;
    *rightSpeedSetpoint = 0;
    *deltaRadius = 0;
    *leftPWM = 0;
    *rightPWM = 0;

    leftMotor.run(0);
    rightMotor.run(0);

    translationPID.resetErrors();
    leftSpeedPID.resetErrors();
    rightSpeedPID.resetErrors();
    curvePID.resetErrors();

    moving = false;
    controlled = true;

    *stahp = false;
}

void MotionController::setSpeedTranslation(int speed)
{
    maxSpeedTranslation = ABS(speed);
    maxSpeed = (long)(1.3*ABS(speed));
}

void MotionController::manageStop()
{
    static long time = 0;
  //  static uint32_t time2 = 0;
  //  static uint32_t time3 = 0;
  //  static uint32_t timeToEstablish = 0;
  //  static uint32_t timeNotEstablished = 0;
  //  static bool isSpeedEstablished = false;

    if (isPhysicallyStopped() && moving && controlled) // Pour un blocage classique
    {

        if (time == 0)
        { //D�but du timer
            time = Millis();
        }
        else
        {
            if ((Millis() - time) >= delayToStop)
            { //Si arr�t� plus de 'delayToStop' ms
                if (ABS(translationPID.getError()) <= toleranceTranslation)
                { //Stop� pour cause de fin de mouvement
                   // std::cout << "DEBUG : ARRIVED AT DESTINATION " << translationPID.getError() << " : " << translationSetpoint << std::endl;
                    stop();
                   // moveAbnormal = false;
                }
                else
                { //Stopp� pour blocage
                  //  stop();
                  //  moveAbnormal = true;
                }
            }
        }
    }

    /*else if(moving && !isSpeedEstablished && !forcedMovement && curveMovement){ // V�rifie que le ratio reste bon pdt les traj courbes

        if (leftCurveRatio<rightCurveRatio && averageRightSpeed.value() !=0 && rightCurveRatio!=0){ // si on tourne a gauche
            if (ABS((averageLeftSpeed.value()/averageRightSpeed.value())-(leftCurveRatio/rightCurveRatio))>toleranceCurveRatio){
                stop();
                moveAbnormal = true;
            }
        }
        else if(rightCurveRatio<leftCurveRatio && averageLeftSpeed.value()!=0 && leftCurveRatio!=0){ //si on tourne � droite
            if (ABS((averageRightSpeed.value()/averageLeftSpeed.value())-(rightCurveRatio/leftCurveRatio))>toleranceCurveRatio){
                stop();
                moveAbnormal = true;
            }
        }
    }*/ //SOULD NOT BE USEFUL


 /*   else if ((isLeftWheelSpeedAbnormal() || isRightWheelSpeedAbnormal()) && curveMovement && !forcedMovement) // Sert a v�rifier que les consignes de vitesse sont bien respect�es (blocage pour les trajectoires courbes)
    {
        if (time2 == 0)
        { //D�but du timer
            time2 = Millis();
        }
        else
        {
            if (ABS(translationPID.getError()) <= toleranceTranslation && ABS(rotationPID.getError()) <= toleranceRotation)
            { //Stopp� pour cause de fin de mouvement
                stop();
                isSpeedEstablished = false;
                moveAbnormal = false;
            }
            else if (((Millis() - time2) >= delayToStopCurve) && isSpeedEstablished){

                stop();
                isSpeedEstablished = false;
                moveAbnormal = true;

            }
        }
    }*/

  /*  else if (forcedMovement && moving){

        if (time3 == 0)
        {
            time3 = Millis();
        }
        else
        {
            if ((Millis() - time3) >= delayToStop){
                if (ABS(translationPID.getError()) <= toleranceTranslation && ABS(rotationPID.getError()) <= toleranceRotation)
                { //Stopp� pour cause de fin de mouvement
                    stop();
                    moveAbnormal = false;

                }
            }
        }
    }*/



    else
    {
        time = 0;
    //    time2 =0;
     //   time3 = 0; // Test
    }
}

void MotionController::updatePosition()
{
    static long precedentL(0);
    *x += (*currentDistance - precedentL)*(double)sin(1.57f - (float)*currentAngle)*MM_PER_TICK;
    *y += (*currentDistance - precedentL)*(double)sin((float)*currentAngle)*MM_PER_TICK;
    precedentL = *currentDistance;
}

void MotionController::sweep(bool way) // true >0 ; false <0
{
    sweepRadius = way ? 10000 : -10000;
    sweeping = true;
}

void MotionController::stopSweep(void)
{
    sweeping = false;
    *curveSetpoint = 1000000;
}

void MotionController::compute_direction_table(void)
{
    for(int i=0 ; i<MAX_RADIUS ; i++)
    {
        direction_table[i] = (float) ((1.5707 - atan((float)i / (float)DIST_MOTOR_DIRECTION)));
    }
}

bool MotionController::isPhysicallyStopped() {
    return (translationPID.getDerivativeError() == 0) || (ABS(ABS(leftSpeedPID.getError())-ABS(rightSpeedPID.getError()))>toleranceDifferentielle);
}

const char * MotionController::getTunings(void)
{
    /*return (
            std::string("LEFT SPEED : ")+std::to_string(leftSpeedPID.getKp())+std::string(" ")+std::to_string(leftSpeedPID.getKi())+std::string(" ")+std::to_string(leftSpeedPID.getKd())+std::string("\r\n")+
            std::string("RIGHT SPEED : ")+std::to_string(rightSpeedPID.getKp())+std::string(" ")+std::to_string(rightSpeedPID.getKi())+std::string(" ")+std::to_string(rightSpeedPID.getKd())+std::string("\r\n")+
            std::string("TRANSLATION PID : ")+std::to_string(translationPID.getKp())+std::string(" ")+std::to_string(translationPID.getKi())+std::string(" ")+std::to_string(translationPID.getKd())+std::string("\r\n")+
            std::string("CURVE PID : ")+std::to_string(curvePID.getKp())+std::string(" ")+std::to_string(curvePID.getKi())+std::string(" ")+std::to_string(curvePID.getKd())+std::string("\r\n")
          ).c_str();*/
}

void MotionController::setTranslationTunings(float kp, float ki, float kd)
{
    translationPID.setTunings(kp, ki, kd);
}

void MotionController::setCurveTunings(float kp, float ki, float kd)
{
    curvePID.setTunings(kp, ki, kd);
}

void MotionController::setLeftSpeedTunings(float kp, float ki, float kd)
{
    leftSpeedPID.setTunings(kp, ki, kd);
}

void MotionController::setRightSpeedTunings(float kp, float ki, float kd)
{
    rightSpeedPID.setTunings(kp, ki, kd);
}

void MotionController::testPosition()
{
    orderTranslation(distanceTest);
}

void MotionController::orderTranslation(long mmDistance)
{
    if(!moving)
    {
        translationPID.resetErrors();
        moving = true;
        controlled = true;
    }
    *translationSetpoint += (long) ((double)mmDistance / (double)MM_PER_TICK);
  //  std::cout << "it's me order: " << *translationSetpoint << std::endl;
}

void MotionController::orderCurveRadius(long c)
{
    *curveSetpoint = c;
}

void MotionController::testSpeed(int speed)
{
    /*std::thread t([this, speed](){
        *translationSpeed = speed;
        moving = true;
        controlled = false;

        timespec t, r;
        t.tv_sec= 4;
        t.tv_nsec = 0;
        nanosleep(&t, &r);

        stop();
    });
    t.detach();*/
}

void MotionController::orderAngle(float angle)
{
    direction.setAngle(angle);
}

Odometry* MotionController::getOdometry(void)
{
    return &odo;
}

long MotionController::getCurveRadius(void)
{
    return *currentRadius;
}

void MotionController::setTrajectory(Cinematic list[], long distance)
{
   /* for(Cinematic &i : list)
    {
        pointsToPass.push(i);
    }

    *curveSetpoint = (long)list[0].curvePoint;
    lastWay = list[0].way;
    orderTranslation((lastWay ? 1 : -1)*distance);*/
}


