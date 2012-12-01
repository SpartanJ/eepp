#include <eepp/math/cperlinnoise.hpp>

namespace EE { namespace Math {

cPerlinNoise::cPerlinNoise() {
	Init();
}

cPerlinNoise::~cPerlinNoise() {}

void cPerlinNoise::Init() {
	mPersistence	= 0.25f;
	mOctaves		= 4;
	mFrequency		= 0.015f;
	mAmplitude		= 1;
	mFreqOctaveDep	= false;
	mAmpOctaveDep	= false;
}

eeFloat cPerlinNoise::PerlinNoise2D(eeFloat x, eeFloat y) {
	eeFloat total	= 0;
	eeFloat p		= mPersistence;
	eeFloat n		= static_cast<eeFloat>( mOctaves - 1 );
	eeFloat tmpFreq = mFrequency;
	eeFloat amp		= mAmplitude;
	mCurrSeed		= 1;

	for ( Int32 i = 0; i <= n; i++ ) {
		if ( mFreqOctaveDep ) {
			tmpFreq *= 2;
		}

		if ( mAmpOctaveDep ) {
			amp *= p;
		}

		total = total + InterpolatedNoise2D( x * tmpFreq, y * tmpFreq ) * amp;
	}

	return total;
}

eeFloat cPerlinNoise::Noise2D( Int32 x, Int32 y ) {
	Int32 n = x + y * 57;

	n = ( n << 13 ) ^ n;

	n = ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff );

	return static_cast<eeFloat>( 1.0 - static_cast<eeFloat>( n ) / 1073741824.0);
}

eeFloat cPerlinNoise::SmoothedNoise2D(eeFloat x, eeFloat y) {
	register Int32	tx	= static_cast<Int32>( x );
	register Int32	ty	= static_cast<Int32>( y );

	eeFloat corners = ( Noise2D( tx - 1, ty - 1	) + Noise2D( tx + 1, ty - 1	) + Noise2D( tx - 1	, ty + 1 ) + Noise2D( tx + 1, ty + 1 ) ) / 16;
	eeFloat sides   = ( Noise2D( tx - 1, ty		) + Noise2D( tx + 1, ty		) + Noise2D( tx		, ty - 1 ) + Noise2D( tx	, ty + 1 ) ) /  8;
	eeFloat center  = Noise2D( tx, ty ) / 4;

	return corners + sides + center;
}

eeFloat cPerlinNoise::Interpolate( eeFloat a, eeFloat b, eeFloat x ) {
	eeFloat fac1 = 3 * eepow( 1 - x, 2	) - 2 * eepow( 1 - x, 3 );
	eeFloat fac2 = 3 * eepow( x, 2		) - 2 * eepow( x, 3		);

	return a * fac1 + b * fac2; //add the weighted factors
}

eeFloat cPerlinNoise::InterpolatedNoise2D(eeFloat x, eeFloat y) {
	Int32 eger_X    = static_cast<Int32>( x );
	Int32 eger_Y    = static_cast<Int32>( y );

	eeFloat fractional_X = x - eger_X;
	eeFloat fractional_Y = y - eger_Y;

	eeFloat feger_X = static_cast<eeFloat> ( eger_X );
	eeFloat feger_Y = static_cast<eeFloat> ( eger_Y );

	eeFloat v1 = SmoothedNoise2D( feger_X		, feger_Y		);
	eeFloat v2 = SmoothedNoise2D( feger_X + 1.f	, feger_Y		);
	eeFloat v3 = SmoothedNoise2D( feger_X		, feger_Y + 1.f	);
	eeFloat v4 = SmoothedNoise2D( feger_X + 1.f	, feger_Y + 1.f	);

	eeFloat i1 = Interpolate( v1 , v2 , fractional_X );
	eeFloat i2 = Interpolate( v3 , v4 , fractional_X );

	return Interpolate( i1 , i2 , fractional_Y );
}

}}
