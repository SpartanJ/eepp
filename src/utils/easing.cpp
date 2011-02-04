#include "easing.hpp"

namespace EE { namespace Utils { namespace easing {

easingCbFunc easingCb[] = {
	LinearInterpolation,
	QuadraticIn,
	QuadraticOut,
	QuadraticInOut,
	SineIn,
	SineOut,
	SineInOut,
	ExponentialIn,
	ExponentialOut,
	ExponentialInOut,
	QuarticIn,
	QuarticOut,
	QuarticInOut,
	QuinticIn,
	QuinticOut,
	QuinticInOut,
	CircularIn,
	CircularOut,
	CircularInOut,
	CubicIn,
	CubicOut,
	CubicInOut,
	BackIn,
	BackOut,
	BackInOut,
	BounceIn,
	BounceOut,
	BounceInOut,
	ElasticIn,
	ElasticOut,
	ElasticInOut
};

eeFloat LinearInterpolation( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	return b + t * c / d;
}

eeFloat QuadraticIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d;

	return c * t * t + b;
}

eeFloat QuadraticOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d;

	return -c * t * ( t - 2 ) + b;
}

eeFloat QuadraticInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d / 2;

	if ( t < 1 )
		return c / 2 * t * t + b;

	--t;

	return -c / 2 * ( t * ( t - 2 ) - 1 ) + b;
}

eeFloat SineIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	return -c * eecos( t / d * ( PI / 2 ) ) + c + b;
}

eeFloat SineOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	return c * eesin( t / d * ( PI / 2 ) ) + b;
}

eeFloat SineInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	return -c / 2 * ( eecos( PI * t / d ) - 1 ) + b;
}

eeFloat ExponentialIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	return t == 0 ? b : c * eepow( 2, 10 * ( t / d - 1 ) ) + b;
}

eeFloat ExponentialOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	return t == d ? b + c : c * ( -eepow( 2, -10 * t / d ) + 1 ) + b;
}

eeFloat ExponentialInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	if (t == 0)
		return b;

	if (t == d)
		return b + c;

	if ( ( t /= d / 2 ) < 1 )
		return c / 2 * eepow( 2, 10 * (t - 1) ) + b;

	return c / 2 * ( -eepow( 2, -10 * --t ) + 2 ) + b;
}

eeFloat QuarticIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d;

	return c * t * t * t * t + b;
}

eeFloat QuarticOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t = t / d - 1;

	return -c * ( t * t * t * t - 1 ) + b;
}

eeFloat QuarticInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d / 2;

	if ( t < 1)
		return c / 2 * t * t * t * t + b;

	t -= 2;

	return -c / 2 * ( t * t * t * t - 2 ) + b;
}

eeFloat QuinticIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d;

	return c * t * t * t * t * t + b;
}

eeFloat QuinticOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t = t / d - 1;

	return c * ( t * t * t * t * t + 1) + b;
}

eeFloat QuinticInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d / 2;

	if ( t < 1 )
		return c / 2 * t * t * t * t * t + b;

	t -= 2;

	return c / 2 * ( t * t * t * t * t + 2) + b;
}

eeFloat CircularIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d;

	return -c * ( eesqrt( 1 - t * t ) - 1) + b;
}

eeFloat CircularOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t = t / d - 1;

	return c * eesqrt( 1 - t * t ) + b;
}

eeFloat CircularInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d / 2;

	if ( t < 1 )
		return -c / 2 * ( eesqrt( 1 - t * t ) - 1 ) + b;

	t -= 2;

	return c / 2 * ( eesqrt( 1 - t * t ) + 1) + b;
}

eeFloat CubicIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d;

	return c * t * t * t + b;
}

eeFloat CubicOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t = t / d - 1;

	return c * ( t * t * t + 1) + b;
}

eeFloat CubicInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	t /= d / 2;

	if ( t < 1 )
		return c / 2 * t * t * t + b;

	t -= 2;

	return c / 2 * ( t * t * t + 2 ) + b;
}

eeFloat BackIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	eeFloat s = 1.70158f;

	t /= d;

	return c * t * t * ( ( s + 1 ) * t - s) + b;
}

eeFloat BackOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	eeFloat	s = 1.70158f;

	t = t / d - 1;

	return c * ( t * t * ( ( s + 1 ) * t + s ) + 1 ) + b;
}

eeFloat BackInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	float s = 1.70158f;

	t /= d / 2;
	s *= ( 1.525f );

	if ( t < 1 )
		return c / 2 * ( t * t * ( ( s + 1 ) * t - s ) ) + b;

	t -= 2;

	return c / 2 * ( t * t * ( ( s + 1 ) * t + s ) + 2) + b;
}

eeFloat BounceOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
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

eeFloat BounceIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	return c - BounceOut( d - t, 0, c, d ) + b;
}

eeFloat BounceInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	if ( t < d * 0.5f )
		return BounceIn( t * 2.f, 0.f, c, d ) * 0.5f + b;

	return BounceOut( t * 2.f - d, 0.f, c, d ) * 0.5f + c * 0.5f + b;
}

eeFloat ElasticIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	if ( t == 0.f )
		return b;

	t /= d;

	if ( t == 1.f )
		return b + c;

	eeFloat p = d * 0.3f;
	eeFloat s = p / 4.f;
	eeFloat a = c;

	t -= 1.f;

	return -( a * eepow( 2.f , 10.f * t ) * eesin( ( t * d - s ) * ( 2.f * PI ) / p ) ) + b;
}

eeFloat ElasticOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	if ( t == 0.f )
		return b;

	t /= d;

	if ( t ==1 )
		return b + c;

	eeFloat p = d * 0.3f;
	eeFloat s = p / 4.f;
	eeFloat a = c;

	return ( a * eepow( 2.f, -10.f * t ) * eesin( ( t * d - s ) * ( 2.f * PI ) / p ) + c + b );
}

eeFloat ElasticInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d ) {
	if ( t == 0 )
		return b;

	t /= d / 2;

	if ( t == 2 )
		return b + c;

	eeFloat p = d * ( 0.3f * 1.5f );
	eeFloat a = c;
	eeFloat s = p / 4.f;

	if ( t < 1 ) {
		t -= 1.f;

		return -0.5f * ( a * eepow( 2.f, 10.f * t ) * eesin( ( t * d - s ) * ( 2.f * PI ) / p ) ) + b;
	}

	t -= 1.f;

	return a * eepow( 2.f, -10.f * t ) * eesin( ( t * d - s ) * ( 2.f * PI ) / p ) * 0.5f + c + b;
}

}}}
