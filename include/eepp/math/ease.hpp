#ifndef EE_MATH_EASE
#define EE_MATH_EASE

#include <string>

namespace EE { namespace Math {

/** @brief Container class of the Interpolation types. */
class Ease {
  public:
	/** @brief Interpolation Define the type of interpolation used. */
	enum Interpolation {
		Linear,
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
		ElasticInOut,
		None
	};

	static Interpolation fromName( const std::string& name,
								   const Interpolation& defaultInterpolation = Ease::Linear ) {
		if ( "linear" == name )
			return Ease::Linear;
		if ( "quadraticin" == name || "quadratic-in" == name )
			return Ease::QuadraticIn;
		if ( "quadraticout" == name || "quadratic-out" == name )
			return Ease::QuadraticOut;
		if ( "quadraticinout" == name || "quadratic-in-out" == name )
			return Ease::QuadraticInOut;
		if ( "sinein" == name || "sine-in" == name )
			return Ease::SineIn;
		if ( "sineout" == name || "sine-out" == name )
			return Ease::SineOut;
		if ( "sineinout" == name || "sine-in-out" == name )
			return Ease::SineInOut;
		if ( "exponentialin" == name || "exponential-in" == name )
			return Ease::ExponentialIn;
		if ( "exponentialout" == name || "exponential-out" == name )
			return Ease::ExponentialOut;
		if ( "exponentialinout" == name || "exponential-in-out" == name )
			return Ease::ExponentialInOut;
		if ( "quarticin" == name || "quartic-in" == name )
			return Ease::QuarticIn;
		if ( "quarticout" == name || "quartic-out" == name )
			return Ease::QuarticOut;
		if ( "quarticinout" == name || "quartic-in-out" == name )
			return Ease::QuarticInOut;
		if ( "quinticin" == name || "quintic-in" == name )
			return Ease::QuinticIn;
		if ( "quinticout" == name || "quintic-out" == name )
			return Ease::QuinticOut;
		if ( "quinticinout" == name || "quintic-in-out" == name )
			return Ease::QuinticInOut;
		if ( "circularin" == name || "circular-in" == name )
			return Ease::CircularIn;
		if ( "circularout" == name || "circular-out" == name )
			return Ease::CircularOut;
		if ( "circularinout" == name || "circular-in-out" == name )
			return Ease::CircularInOut;
		if ( "cubicin" == name || "cubic-in" == name )
			return Ease::CubicIn;
		if ( "cubicout" == name || "cubic-out" == name )
			return Ease::CubicOut;
		if ( "cubicinout" == name || "cubic-in-out" == name )
			return Ease::CubicInOut;
		if ( "backin" == name || "back-in" == name )
			return Ease::BackIn;
		if ( "backout" == name || "back-out" == name )
			return Ease::BackOut;
		if ( "backinout" == name || "back-in-out" == name )
			return Ease::BackInOut;
		if ( "bouncein" == name || "bounce-in" == name )
			return Ease::BounceIn;
		if ( "bounceout" == name || "bounce-out" == name )
			return Ease::BounceOut;
		if ( "bounceinout" == name || "bounce-in-out" == name )
			return Ease::BounceInOut;
		if ( "elasticin" == name || "elastic-in" == name )
			return Ease::ElasticIn;
		if ( "elasticout" == name || "elastic-out" == name )
			return Ease::ElasticOut;
		if ( "elasticinout" == name || "elastic-in-out" == name )
			return Ease::ElasticInOut;
		return defaultInterpolation;
	}
};

}} // namespace EE::Math

#endif
