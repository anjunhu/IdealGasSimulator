/*
 * lab1util.h
 *
 *  Created on: Jan 20, 2021
 *      Author: ah
 */

#ifndef INC_LAB1UTIL_H_
#define INC_LAB1UTIL_H_

struct KalmanState{
	float q; //process noise covariance
	float r; //measurement noise covariance
	float x; //estimated value
	float p; //estimation error covariance
	float k; // adaptive Kalman filter gain.;
};

extern int kalmanFilterA (float* InputArray, float* OutputArray, struct KalmanState* kstate, int length);

#endif /* INC_LAB1UTIL_H_ */
