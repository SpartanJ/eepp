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

	static std::string toString( const Interpolation& interpolation ) {
		switch ( interpolation ) {
			case Ease::Linear:
				return "linear";
			case Ease::QuadraticIn:
				return "quadratic-in";
			case Ease::QuadraticOut:
				return "quadratic-out";
			case Ease::QuadraticInOut:
				return "quadratic-in-out";
			case Ease::SineIn:
				return "sine-in";
			case Ease::SineOut:
				return "sine-out";
			case Ease::SineInOut:
				return "sine-in-out";
			case Ease::ExponentialIn:
				return "exponential-in";
			case Ease::ExponentialOut:
				return "exponential-out";
			case Ease::ExponentialInOut:
				return "exponential-in-out";
			case Ease::QuarticIn:
				return "quartic-in";
			case Ease::QuarticOut:
				return "quartic-out";
			case Ease::QuarticInOut:
				return "quartic-in-out";
			case Ease::QuinticIn:
				return "quintic-in";
			case Ease::QuinticOut:
				return "quintic-out";
			case Ease::QuinticInOut:
				return "quintic-in-out";
			case Ease::CircularIn:
				return "circular-in";
			case Ease::CircularOut:
				return "circular-out";
			case Ease::CircularInOut:
				return "circular-in-out";
			case Ease::CubicIn:
				return "cubic-in";
			case Ease::CubicOut:
				return "cubic-out";
			case Ease::CubicInOut:
				return "cubic-in-out";
			case Ease::BackIn:
				return "back-in";
			case Ease::BackOut:
				return "back-out";
			case Ease::BackInOut:
				return "back-in-out";
			case Ease::BounceIn:
				return "bounce-in";
			case Ease::BounceOut:
				return "bounce-out";
			case Ease::BounceInOut:
				return "bounce-in-out";
			case Ease::ElasticIn:
				return "elastic-in";
			case Ease::ElasticOut:
				return "elastic-out";
			case Ease::ElasticInOut:
				return "elastic-in-out";
			case Ease::None:
				return "none";
		}
		return "";
	}

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
