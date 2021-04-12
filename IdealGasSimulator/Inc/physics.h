/*
 * physics.h
 */

#ifndef INC_PHYSICS_H_
#define INC_PHYSICS_H_

struct KalmanState{
	float q; //process noise covariance
	float r; //measurement noise covariance
	float x; //estimated value
	float p; //estimation error covariance
	float k; // adaptive Kalman filter gain.;
};

extern float kalmanFilterA (struct KalmanState* ks, float measurement);

float gyro2work (float* gyro){
	float gyro_dps[3];
	gyro_dps[0] = gyro[0]/1000;
	gyro_dps[1] = gyro[1]/1000;
	gyro_dps[2] = gyro[2]/1000;
	float inertia = 0.001*0.5*0.5; // based on standard atomic weight
	return 0.5*inertia*(gyro_dps[0]*gyro_dps[0] + gyro_dps[1]*gyro_dps[1]+gyro_dps[2]*gyro_dps[2]);
}

void leftRotateOne(float* InputArray, int length)
{
    int temp = InputArray[0], i;
    for (i = 0; i < length - 1; i++)
        InputArray[i] = InputArray[i + 1];
    InputArray[length-1] = temp;
}

#endif /* INC_PHYSICS_H_ */
