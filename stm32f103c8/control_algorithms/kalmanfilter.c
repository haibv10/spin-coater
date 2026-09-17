#include "kalmanfilter.h"

void kalman_filter_init(kalman_filter *kf, float a, float h, float q,
		       float r, float initial_x, float initial_p)
{
	kf->A = a;
	kf->H = h;
	kf->Q = q;
	kf->R = r;
	kf->x = initial_x;
	kf->P = initial_p;
	kf->initialized = 1;
}

float kalman_filter_update(kalman_filter *kf, float measurement)
{
	kf->x = kf->A * kf->x;
	kf->P = kf->A * kf->P *
					     kf->A +
				     kf->Q;
	float kalman_gain = kf->P * kf->H /
		  (kf->H * kf->P *
			   kf->H +
		   kf->R);
	kf->x =
		kf->x +
		kalman_gain * (measurement - kf->H * kf->x);
	kf->P =
		(1 - kalman_gain * kf->H) * kf->P;
	return kf->x;
}
