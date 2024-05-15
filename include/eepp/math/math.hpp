#ifndef EE_MATH_HPP
#define EE_MATH_HPP

#include <cmath>
#include <cstdlib>
#include <eepp/config.hpp>

namespace EE { namespace Math {

/** Set a Random Seed to the Randomizer */
inline Uint32 setRandomSeed( Uint32 seed ) {
	srand( seed );
	return seed;
}

/** Generate a floating point random number
 * @param fMin The minimun value
 * @param fMax the maximun value
 * @return The random number generated
 */
inline Float randf( const Float& fMin = 0.0f, const Float& fMax = 1.0f ) {
	return ( fMin + ( fMax - fMin ) * ( rand() / ( (Float)RAND_MAX + 1 ) ) );
}

/** Generate a integer random number
 * @param fMin The minimun value
 * @param fMax the maximun value
 * @return The random number generated
 */
inline int randi( const int& fMin = 0, const int& fMax = 1 ) {
	return (int)( fMin + ( fMax - fMin + 1 ) * ( rand() / ( (Float)RAND_MAX + 1 ) ) );
}

/** Cosine from an Angle in Degress */
inline Float cosAng( const Float& Ang ) {
	return eecos( Ang * EE_PI_180 );
}
/** Sinus from an Angle in Degress */
inline Float sinAng( const Float& Ang ) {
	return eesin( Ang * EE_PI_180 );
}

/** Tangen from an Angle in Degress */
inline Float tanAng( const Float& Ang ) {
	return tan( Ang * EE_PI_180 );
}

/** Convert an Angle from Degrees to Radians */
inline Float radians( const Float& Ang ) {
	return Ang * EE_PI_180;
}

/** Convert an Angle from Math::Radians to Degrees */
inline Float degrees( const Float& Radians ) {
	return Radians * EE_180_PI;
}

/** @return The next power of two of the given Size */
template <typename T> T nextPowOfTwo( T Size ) {
	T p = 1;

	while ( p < Size )
		p <<= 1;

	return p;
}

/** @return If the number given is power of two */
template <typename T> T isPow2( T v ) {
	return ( ( v & ( v - 1 ) ) == 0 );
}

/** Round the number */
template <typename T> inline T round( T r ) {
	return ( r > 0.0f ) ? floor( r + 0.5f ) : ceil( r - 0.5f );
}

/** Round the number always to the upper value */
template <typename T> inline T roundUp( T r ) {
	return ( r > 0.0f ) ? ceil( r ) : ceil( r - 0.5f );
}

/** Round the number always to the lower value */
template <typename T> inline T roundDown( T x ) {
	T ipart;
	T fpart = std::modf( x, &ipart );
	if ( fpart != 0.0 )
		return fpart <= 0.5 ? std::floor( x ) : std::ceil( x );
	return std::round( x );
}

/** @return The number of digits in a number. */
template <typename T> static T countDigits( T num ) {
	return num == 0 ? 1 : (T)log10( std::abs( num ) ) + 1;
}

}} // namespace EE::Math

#endif
