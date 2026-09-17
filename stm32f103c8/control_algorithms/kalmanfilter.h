#ifndef __KALMANFILTER_H__
#define __KALMANFILTER_H__

typedef struct {
	float A;
	float B;
	float H;
	float Q;
	float R;

	float x;
	float P;
	int initialized;
} kalman_filter;

void kalman_filter_init(kalman_filter *kf, float a, float h, float q,
		       float r, float initial_x, float initial_p);
float kalman_filter_update(kalman_filter *kf, float measurement);

#endif /* __KALMANFILTER_H__ */
