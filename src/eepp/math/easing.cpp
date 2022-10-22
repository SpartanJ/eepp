#include <eepp/math/easing.hpp>

namespace EE { namespace Math { namespace easing {

easingCbFunc easingCb[] = { linearInterpolation,
							quadraticIn,
							quadraticOut,
							quadraticInOut,
							sineIn,
							sineOut,
							sineInOut,
							exponentialIn,
							exponentialOut,
							exponentialInOut,
							quarticIn,
							quarticOut,
							quarticInOut,
							quinticIn,
							quinticOut,
							quinticInOut,
							circularIn,
							circularOut,
							circularInOut,
							cubicIn,
							cubicOut,
							cubicInOut,
							backIn,
							backOut,
							backInOut,
							bounceIn,
							bounceOut,
							bounceInOut,
							elasticIn,
							elasticOut,
							elasticInOut,
							cubicBezierNoParams,
							noneInterpolation };

/**
 * https://github.com/gre/bezier-easing
 * BezierEasing - use bezier curve for transition easing function
 * by Gaëtan Renaudeau 2014 - 2015 – MIT License
 */
#define NEWTON_ITERATIONS 4
#define NEWTON_MIN_SLOPE 0.001
#define SUBDIVISION_PRECISION 0.0000001
#define SUBDIVISION_MAX_ITERATIONS 10
#define kSplineTableSize 11
#define kSampleStepSize ( 1.0 / ( kSplineTableSize - 1.0 ) )

double A( double aA1, double aA2 ) {
	return 1.0 - 3.0 * aA2 + 3.0 * aA1;
}
double B( double aA1, double aA2 ) {
	return 3.0 * aA2 - 6.0 * aA1;
}
double C( double aA1 ) {
	return 3.0 * aA1;
}

// Returns x(t) given t, x1, and x2, or y(t) given t, y1, and y2.
double calcBezier( double aT, double aA1, double aA2 ) {
	return ( ( A( aA1, aA2 ) * aT + B( aA1, aA2 ) ) * aT + C( aA1 ) ) * aT;
}

// Returns dx/dt given t, x1, and x2, or dy/dt given t, y1, and y2.
double getSlope( double aT, double aA1, double aA2 ) {
	return 3.0 * A( aA1, aA2 ) * aT * aT + 2.0 * B( aA1, aA2 ) * aT + C( aA1 );
}

double binarySubdivide( double aX, double aA, double aB, double mX1, double mX2 ) {
	double currentX, currentT;
	size_t i = 0;
	do {
		currentT = aA + ( aB - aA ) / 2.0;
		currentX = calcBezier( currentT, mX1, mX2 ) - aX;
		if ( currentX > 0.0 ) {
			aB = currentT;
		} else {
			aA = currentT;
		}
	} while ( eeabs( currentX ) > SUBDIVISION_PRECISION && ++i < SUBDIVISION_MAX_ITERATIONS );
	return currentT;
}

double newtonRaphsonIterate( double aX, double aGuessT, double mX1, double mX2 ) {
	for ( size_t i = 0; i < NEWTON_ITERATIONS; ++i ) {
		double currentSlope = getSlope( aGuessT, mX1, mX2 );
		if ( currentSlope == 0.0 ) {
			return aGuessT;
		}
		double currentX = calcBezier( aGuessT, mX1, mX2 ) - aX;
		aGuessT -= currentX / currentSlope;
	}
	return aGuessT;
}

double cubicBezierInterpolation( double x1, double y1, double x2, double y2, double t ) {
	if ( !( 0 <= x1 && x1 <= 1 && 0 <= x2 && x2 <= 1 ) )
		return t; // 'bezier x values must be in [0, 1] range'

	if ( x1 == y1 && x2 == y2 )
		return t;

	// Precompute samples table
	double sampleValues[kSplineTableSize];
	for ( size_t i = 0; i < kSplineTableSize; ++i )
		sampleValues[i] = calcBezier( i * kSampleStepSize, x1, x2 );

	auto getTForX = [&sampleValues, x1, x2]( double aX ) {
		double intervalStart = 0.0;
		size_t currentSample = 1;
		double lastSample = kSplineTableSize - 1;

		for ( ; currentSample != lastSample && sampleValues[currentSample] <= aX;
			  ++currentSample ) {
			intervalStart += kSampleStepSize;
		}
		--currentSample;

		// Interpolate to provide an initial guess for t
		double dist = ( aX - sampleValues[currentSample] ) /
					  ( sampleValues[currentSample + 1] - sampleValues[currentSample] );
		double guessForT = intervalStart + dist * kSampleStepSize;

		double initialSlope = getSlope( guessForT, x1, x2 );
		if ( initialSlope >= NEWTON_MIN_SLOPE ) {
			return newtonRaphsonIterate( aX, guessForT, x1, x2 );
		} else if ( initialSlope == 0.0 ) {
			return guessForT;
		} else {
			return binarySubdivide( aX, intervalStart, intervalStart + kSampleStepSize, x1, x2 );
		}
	};

	// Because JavaScript number are imprecise, we should guarantee the extremes are right.
	if ( t == 0 || t == 1 )
		return t;
	return calcBezier( getTForX( t ), y1, y2 );
}

}}} // namespace EE::Math::easing
