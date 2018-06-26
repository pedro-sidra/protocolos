#ifndef CONTROLLER_H

#define CONTROLLER_H
class Controller
{
public:
	Controller(float Kp, float Ki, float Kd,float ts);
	float update(float PV);
	void reset();
	void setSP(float);
	void setIntegrator(bool tf);

private:
	float _error;
	float _iError;
	float _dError;
	float _errorLast;

	float _SP;

	float _ts;
	float _Kp;
	float _Ki;
	float _Kd;

	bool _integrate;
};


#endif
