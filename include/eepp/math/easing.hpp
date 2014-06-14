#ifndef EE_MATH_EASINGEASING_H
#define EE_MATH_EASINGEASING_H

#include <eepp/config.hpp>
#include <cmath>

namespace EE { namespace Math { namespace easing {

typedef double( *easingCbFunc )( double, double, double, double );

extern EE_API easingCbFunc easingCb[];

/**
*  Calculate the position of a point from a linear interpolation.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/
inline double LinearInterpolation( double t, double b, double c, double d ) {
	return b + t * c / d;
}

/**
*  The <code>QuadraticIn()</code> method starts motion from a zero velocity
*  and then accelerates motion as it executes.
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/
inline double QuadraticIn( double t, double b, double c, double d ) {
	t /= d;

	return c * t * t + b;
}

/**
*  The <code>QuadraticOut()</code> method starts motion fast
*  and then decelerates motion to a zero velocity as it executes.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/
inline double QuadraticOut( double t, double b, double c, double d ) {
	t /= d;

	return -c * t * ( t - 2 ) + b;
}

/**
*  The <code>QuadraticInOut()</code> method combines the motion
*  of the <code>QuadraticIn()</code> and <code>QuadraticOut()</code> methods
*  to start the motion from a zero velocity,
*  accelerate motion, then decelerate to a zero velocity.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/
inline double QuadraticInOut( double t, double b, double c, double d ) {
	t /= d / 2;

	if ( t < 1 )
		return c / 2 * t * t + b;

	--t;

	return -c / 2 * ( t * ( t - 2 ) - 1 ) + b;
}

/**
*  The <code>SineIn()</code> method starts motion from zero velocity
*  and then accelerates motion as it executes.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/
inline double SineIn( double t, double b, double c, double d ) {
	return -c * eecos( t / d * ( EE_PI / 2 ) ) + c + b;
}

/**
*  The <code>easeOut()</code> method starts motion fast
*  and then decelerates motion to a zero velocity as it executes.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/
inline double SineOut( double t, double b, double c, double d ) {
	return c * eesin( t / d * ( EE_PI / 2 ) ) + b;
}

/**
*  The <code>easeInOut()</code> method combines the motion
*  of the <code>easeIn()</code> and <code>easeOut()</code> methods
*  to start the motion from a zero velocity, accelerate motion,
*  then decelerate to a zero velocity.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/
inline double SineInOut( double t, double b, double c, double d ) {
	return -c / 2 * ( eecos( EE_PI * t / d ) - 1 ) + b;
}

/**
*  The <code>ExponentialIn()</code> method starts motion slowly
*  and then accelerates motion as it executes.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial position of a component.
*  @param c Specifies the total change in position of the component.
*  @param d Specifies the duration of the effect, in milliseconds.
*  @return Number corresponding to the position of the component.
*/
inline double ExponentialIn( double t, double b, double c, double d ) {
	return t == 0 ? b : c * eepow( 2, 10 * ( t / d - 1 ) ) + b;
}

/**
*  The <code>ExponentialOut()</code> method starts motion fast
*  and then decelerates motion to a zero velocity as it executes.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/
inline double ExponentialOut( double t, double b, double c, double d ) {
	return t == d ? b + c : c * ( -eepow( 2, -10 * t / d ) + 1 ) + b;
}

/**
*  The <code>ExponentialInOut()</code> method combines the motion
*  of the <code>ExponentialIn()</code> and <code>ExponentialOut()</code> methods
*  to start the motion from a zero velocity, accelerate motion,
*  then decelerate to a zero velocity.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/
inline double ExponentialInOut( double t, double b, double c, double d ) {
	if (t == 0)
		return b;

	if (t == d)
		return b + c;

	if ( ( t /= d / 2 ) < 1 )
		return c / 2 * eepow( 2, 10 * (t - 1) ) + b;

	return c / 2 * ( -eepow( 2, -10 * --t ) + 2 ) + b;
}

inline double QuarticIn( double t, double b, double c, double d ) {
	t /= d;

	return c * t * t * t * t + b;
}

inline double QuarticOut( double t, double b, double c, double d ) {
	t = t / d - 1;

	return -c * ( t * t * t * t - 1 ) + b;
}

inline double QuarticInOut( double t, double b, double c, double d ) {
	t /= d / 2;

	if ( t < 1)
		return c / 2 * t * t * t * t + b;

	t -= 2;

	return -c / 2 * ( t * t * t * t - 2 ) + b;
}

inline double QuinticIn( double t, double b, double c, double d ) {
	t /= d;

	return c * t * t * t * t * t + b;
}

inline double QuinticOut( double t, double b, double c, double d ) {
	t = t / d - 1;

	return c * ( t * t * t * t * t + 1) + b;
}

inline double QuinticInOut( double t, double b, double c, double d ) {
	t /= d / 2;

	if ( t < 1 )
		return c / 2 * t * t * t * t * t + b;

	t -= 2;

	return c / 2 * ( t * t * t * t * t + 2) + b;
}

inline double CircularIn( double t, double b, double c, double d ) {
	t /= d;

	return -c * ( eesqrt( 1 - t * t ) - 1) + b;
}

inline double CircularOut( double t, double b, double c, double d ) {
	t = t / d - 1;

	return c * eesqrt( 1 - t * t ) + b;
}

inline double CircularInOut( double t, double b, double c, double d ) {
	t /= d / 2;

	if ( t < 1 )
		return -c / 2 * ( eesqrt( 1 - t * t ) - 1 ) + b;

	t -= 2;

	return c / 2 * ( eesqrt( 1 - t * t ) + 1) + b;
}

inline double CubicIn( double t, double b, double c, double d ) {
	t /= d;

	return c * t * t * t + b;
}

inline double CubicOut( double t, double b, double c, double d ) {
	t = t / d - 1;

	return c * ( t * t * t + 1) + b;
}

inline double CubicInOut( double t, double b, double c, double d ) {
	t /= d / 2;

	if ( t < 1 )
		return c / 2 * t * t * t + b;

	t -= 2;

	return c / 2 * ( t * t * t + 2 ) + b;
}

inline double BackIn( double t, double b, double c, double d ) {
	double s = 1.70158f;

	t /= d;

	return c * t * t * ( ( s + 1 ) * t - s) + b;
}

inline double BackOut( double t, double b, double c, double d ) {
	double	s = 1.70158f;

	t = t / d - 1;

	return c * ( t * t * ( ( s + 1 ) * t + s ) + 1 ) + b;
}

inline double BackInOut( double t, double b, double c, double d ) {
	float s = 1.70158f;

	t /= d / 2;
	s *= ( 1.525f );

	if ( t < 1 )
		return c / 2 * ( t * t * ( ( s + 1 ) * t - s ) ) + b;

	t -= 2;

	return c / 2 * ( t * t * ( ( s + 1 ) * t + s ) + 2) + b;
}

inline double BounceOut( double t, double b, double c, double d ) {
	t /= d;

	if ( t < ( 1.f / 2.75f ) ) {
		return c * ( 7.5625f * t * t ) + b;
	} else if ( t < ( 2.f / 2.75f) ) {
		t -= ( 1.5f / 2.75f );

		return c * ( 7.5625f * t * t + 0.75f ) + b;
	} else if (t < ( 2.5f / 2.75f ) ) {
		t -= ( 2.25f / 2.75f );

		return c * ( 7.5625f * t * t + 0.9375f ) + b;
	} else {
		t -= ( 2.625f / 2.75f );

		return c * ( 7.5625f * t * t + 0.984375f ) + b;
	}
}

inline double BounceIn( double t, double b, double c, double d ) {
	return c - BounceOut( d - t, 0, c, d ) + b;
}

inline double BounceInOut( double t, double b, double c, double d ) {
	if ( t < d * 0.5f )
		return BounceIn( t * 2.f, 0.f, c, d ) * 0.5f + b;

	return BounceOut( t * 2.f - d, 0.f, c, d ) * 0.5f + c * 0.5f + b;
}

inline double ElasticIn( double t, double b, double c, double d ) {
	if ( t == 0.f )
		return b;

	t /= d;

	if ( t == 1.f )
		return b + c;

	double p = d * 0.3f;
	double s = p / 4.f;
	double a = c;

	t -= 1.f;

	return -( a * eepow( 2.f , 10.f * t ) * eesin( ( t * d - s ) * ( 2.f * EE_PI ) / p ) ) + b;
}

inline double ElasticOut( double t, double b, double c, double d ) {
	if ( t == 0.f )
		return b;

	t /= d;

	if ( t ==1 )
		return b + c;

	double p = d * 0.3f;
	double s = p / 4.f;
	double a = c;

	return ( a * eepow( 2.f, -10.f * t ) * eesin( ( t * d - s ) * ( 2.f * EE_PI ) / p ) + c + b );
}

inline double ElasticInOut( double t, double b, double c, double d ) {
	if ( t == 0 )
		return b;

	t /= d / 2;

	if ( t == 2 )
		return b + c;

	double p = d * ( 0.3f * 1.5f );
	double a = c;
	double s = p / 4.f;

	if ( t < 1 ) {
		t -= 1.f;

		return -0.5f * ( a * eepow( 2.f, 10.f * t ) * eesin( ( t * d - s ) * ( 2.f * EE_PI ) / p ) ) + b;
	}

	t -= 1.f;

	return a * eepow( 2.f, -10.f * t ) * eesin( ( t * d - s ) * ( 2.f * EE_PI ) / p ) * 0.5f + c + b;
}

}}}

#endif
