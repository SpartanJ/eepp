#ifndef EE_WINDOW_PIXELDENSITYPOLICY_HPP
#define EE_WINDOW_PIXELDENSITYPOLICY_HPP

#include <algorithm>
#include <cmath>
#include <eepp/config.hpp>

namespace EE { namespace Window { namespace Private {

inline Float roundAutomaticPixelDensity( Float density ) {
	Float whole;
	const Float fraction = std::modf( density, &whole );
	return std::max<Float>( 0.25f, whole + std::round( fraction * 4.f ) * 0.25f );
}

inline Float estimateBestFitPixelDensity( Float ppi, Float diagonalInches, Float logicalScale,
										  bool rounding ) {
	if ( !( ppi >= 40.f && ppi <= 400.f && diagonalInches >= 8.f && diagonalInches <= 100.f ) )
		return 0.f;
	// Larger displays are usually viewed farther away. Limit the adjustment to desk sizes.
	const Float reference =
		std::clamp<Float>( 100.f - 4.f * ( diagonalInches - 20.f ), 60.f, 100.f );
	const Float density = std::clamp<Float>( std::max( ppi / reference, logicalScale ), 1.f, 6.f );
	return rounding ? roundAutomaticPixelDensity( density ) : density;
}

}}} // namespace EE::Window::Private

#endif
