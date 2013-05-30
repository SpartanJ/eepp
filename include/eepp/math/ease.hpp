#ifndef EE_MATH_EASE
#define EE_MATH_EASE

namespace EE { namespace Math {

/** @brief Container class of the Interpolation types. */
class Ease {
	public:
		/** @enum Ease::Interpolation Define the type of interpolation used. */
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
			ElasticInOut
		};
};

}}

#endif
