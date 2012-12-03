#ifndef EE_MATH_HPP
#define EE_MATH_HPP

#include <cmath>
#include <cstdlib>
#include <eepp/declares.hpp>

namespace EE { namespace Math {

/** Set a Random Seed to the Randomizer */
inline Uint32 SetRandomSeed( Uint32 seed ) {
	srand(seed);
	return seed;
}

/** Generate a floating point random number
* @param fMin The minimun value
* @param fMax the maximun value
* @return The random number generated
*/
inline eeFloat Randf( const eeFloat& fMin = 0.0f, const eeFloat& fMax = 1.0f ) {
	return (fMin + (fMax - fMin) * ( rand() / ( (eeFloat) RAND_MAX + 1) ) );
}

/** Generate a integer random number
* @param fMin The minimun value
* @param fMax the maximun value
* @return The random number generated
*/
inline eeInt Randi( const eeInt& fMin = 0, const eeInt& fMax = 1 ) {
	return (eeInt)(fMin + (fMax - fMin + 1) * ( rand() / ( (eeFloat) RAND_MAX + 1) ) );
}

/** Cosine from an Angle in Degress */
inline eeFloat cosAng( const eeFloat& Ang ) {
	return eecos(Ang * EE_PI_180);
}
/** Sinus from an Angle in Degress */
inline eeFloat sinAng( const eeFloat& Ang ) {
	return eesin(Ang * EE_PI_180);
}

/** Tangen from an Angle in Degress */
inline eeFloat tanAng( const eeFloat& Ang ) {
	return tan(Ang * EE_PI_180);
}

/** Convert an Angle from Degrees to Radians */
inline eeFloat Radians( const eeFloat& Ang ) {
	return Ang * EE_PI_180;
}

/** Convert an Angle from Math::Radians to Degrees */
inline eeFloat Degrees( const eeFloat& Radians ) {
	return Radians * EE_180_PI;
}

template <typename T>
T NextPowOfTwo( T Size ) {
	T p = 1;

	while ( p < Size )
		p <<= 1;

	return p;
}

template <typename T>
T IsPow2( T v ) {
	return ( ( v & ( v - 1 ) ) == 0 );
}

template <typename T>
inline T Round( T r ) {
	return (r > 0.0f) ? floor(r + 0.5f) : ceil(r - 0.5f);
}

template <typename T >
inline T RoundUp( T r ) {
	return (r > 0.0f) ? ceil(r) : ceil(r - 0.5f);
}

}}

#endif
