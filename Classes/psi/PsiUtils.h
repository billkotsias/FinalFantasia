#pragma once
#include "math.h"

#define RAD_TO_DEGf(X) ((X)*45.f/(float)M_PI_4)
#define DEG_TO_RADf(X) ((X)*(float)M_PI_4/45.f)
#define M_2PI (3.14159265358979323846f*2.f)

namespace psi {
	/// <TODO> : check if fmod is faster in practice
	inline float boundAngleTo(float angle, float bound) {
		while (angle > bound) angle -= M_2PI;
		while (angle < bound - M_2PI) angle += M_2PI;
		return angle;
	}
	inline float boundAngleToPI(float angle) {
		//return angle;
		return boundAngleTo(angle, (float)M_PI);
	}
}