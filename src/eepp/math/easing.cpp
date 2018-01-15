#include <eepp/math/easing.hpp>

namespace EE { namespace Math { namespace easing {

easingCbFunc easingCb[] = {
	linearInterpolation,
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
	elasticInOut
};

}}}
