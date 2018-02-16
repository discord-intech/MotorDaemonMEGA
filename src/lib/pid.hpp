/**
 * pid.hpp
 *
 * Classe PID : impl?mente un r?gulateur PID (proportionnel int?gral d?riv?)
 *
 * Auteur : Paul BERNIER - bernier.pja@gmail.com
 */

#ifndef PID_HPP
#define PID_HPP

#include <stdint.h>
#include <memory>



class PID
{
public:


	PID() {}

    void setPointers(std::shared_ptr<long> input, std::shared_ptr<long> output, std::shared_ptr<long> setPoint)
    {
        this->input = input;
        this->output = output;
        this->setPoint = setPoint;

        setOutputLimits(-2147483647, 2147483647);
        setTunings(0, 0, 0);
        epsilon = 0;
        pre_error = 0;
        derivative = 0;
        integral = 0;
    }

	void compute() {

        if(kp <=0 && ki <=0 && kd <= 0) return;

		long error = (*setPoint) - (*input);
		derivative = error - pre_error;
		integral += error;
		pre_error = error;

		long result = (long)(
				kp * error + ki * integral + kd * derivative);

		//Saturation

		if (result > outMax) {
			result = outMax;
		} else if (result < outMin) {
			result = outMin;
		}



		//Seuillage de la commande
		if (ABS(result) < epsilon)
			result = 0;

		(*output) = result;
	}

	void setTunings(float kp, float ki, float kd) {
		if (kp < 0 || ki < 0 || kd < 0)
			return;

		this->kp = kp;
		this->ki = ki;
		this->kd = kd;
	}

	void setOutputLimits(int32_t min, int32_t max) {
		if (min >= max)
			return;

		outMin = min;
		outMax = max;

		if ((*output) > outMax)
			(*output) = outMax;
		else if ((*output) < outMin)
			(*output) = outMin;
	}

	long getOutputLimit() {
		return outMax;
	}

	void setEpsilon(int32_t seuil) {
		if(seuil < 0)
			return;
		epsilon = seuil;
	}

	long getEpsilon() {
		return epsilon;
	}

	void resetErrors() {
		pre_error = 0;
		integral = 0;
	}
	float getKp()  {
		return kp;
	}
	float getKi()  {
		return ki;
	}
	float getKd()  {
		return kd;
	}

	long getError()  {
		return pre_error;
	}

	long getDerivativeError()  {
		return derivative;
	}

	long getIntegralErrol()  {
		return integral;
	}

    long getPTR()
    {
        return *input;
    }

private:

	float kp;
	float ki;
	float kd;

	std::shared_ptr<long> input; //Valeur du codeur
    std::shared_ptr<long> output; //Output : pwm
    std::shared_ptr<long> setPoint; //Valeur ? atteindre

	long epsilon;
	long outMin, outMax;

    long pre_error;
    long derivative;
    long integral;
};

#endif
