#include <eepp/math/easing.hpp>
#include <algorithm>
#include <cmath>

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
 * by Gaëtan Renaudeau 2014 - 2026 – MIT License
 */

// Solves x(t) = ((2a * t + 3b) * t + 3c) * t = x for t, with x in (0, 1):
// u = 1/t is the largest real root of x·u³ − 3c·u² − 3b·u − 2a = 0
static double solveTForX( double x, double a, double b, double c ) {
	double j = 1 / std::max( c, std::sqrt( x ) );
	double k = x * j;
	double l = k * j;
	double s = c * j;
	double q = b * l;
	double m = s * s + q;
	double h = -s * ( s * s + 1.5 * q ) - a * k * l;
	double D = h * h - m * m * m;
	double v;
	if ( m == 0 || D > 1e-12 * h * h ) {
		// one real root (Cardano)
		double U = -std::cbrt( h < 0 ? h - std::sqrt( D ) : h + std::sqrt( D ) );
		v = U + m / U;
		// triple root (m = h = 0) gives NaN
		if ( std::isnan( v ) )
			v = 0;
	} else {
		// three real roots, take the largest
		double r = std::sqrt( m );
		v = 2 * r * std::cos( std::acos( std::max( -1.0, std::min( 1.0, -h / ( m * r ) ) ) ) / 3 );
	}
	return std::min( 1.0, k / ( v + s ) );
}

double cubicBezierInterpolation( double x1, double y1, double x2, double y2, double t ) {
	if ( !( 0 <= x1 && x1 <= 1 && 0 <= x2 && x2 <= 1 ) )
		return t; // 'bezier x values must be in [0, 1] range'

	if ( x1 == y1 && x2 == y2 )
		return t;

	// t outside (0, 1) saturates to 0 / 1, NaN stays NaN
	if ( std::isnan( t ) )
		return t;
	if ( t <= 0 )
		return 0;
	if ( t >= 1 )
		return 1;

	// x(t) = ((2a * t + 3b) * t + 3c) * t with a = (3x1 - 3x2 + 1) / 2, b = x2 - 2x1, c = x1
	double u = solveTForX( t, ( 3 * x1 - 3 * x2 + 1 ) / 2, x2 - 2 * x1, x1 );
	return ( ( ( 3 * y1 - 3 * y2 + 1 ) * u + 3 * ( y2 - 2 * y1 ) ) * u + 3 * y1 ) * u;
}

}}} // namespace EE::Math::easing
