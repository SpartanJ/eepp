#include <eepp/math/easing.hpp>

namespace EE { namespace Math { namespace easing {

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

}}}
