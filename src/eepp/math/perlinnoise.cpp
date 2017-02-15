#include <eepp/math/perlinnoise.hpp>
#include <cmath>

namespace EE { namespace Math {

PerlinNoise::PerlinNoise() {
	init();
}

PerlinNoise::~PerlinNoise() {}

void PerlinNoise::init() {
	mPersistence	= 0.25f;
	mOctaves		= 4;
	mFrequency		= 0.015f;
	mAmplitude		= 1;
	mFreqOctaveDep	= false;
	mAmpOctaveDep	= false;
}

Float PerlinNoise::perlinNoise2D(Float x, Float y) {
	Float total	= 0;
	Float p		= mPersistence;
	Float n		= static_cast<Float>( mOctaves - 1 );
	Float tmpFreq = mFrequency;
	Float amp		= mAmplitude;
	mCurrSeed		= 1;

	for ( Int32 i = 0; i <= n; i++ ) {
		if ( mFreqOctaveDep ) {
			tmpFreq *= 2;
		}

		if ( mAmpOctaveDep ) {
			amp *= p;
		}

		total = total + interpolatedNoise2D( x * tmpFreq, y * tmpFreq ) * amp;
	}

	return total;
}

Float PerlinNoise::noise2D( Int32 x, Int32 y ) {
	Int32 n = x + y * 57;

	n = ( n << 13 ) ^ n;

	n = ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff );

	return static_cast<Float>( 1.0 - static_cast<Float>( n ) / 1073741824.0);
}

Float PerlinNoise::smoothedNoise2D(Float x, Float y) {
	register Int32	tx	= static_cast<Int32>( x );
	register Int32	ty	= static_cast<Int32>( y );

	Float corners = ( noise2D( tx - 1, ty - 1	) + noise2D( tx + 1, ty - 1	) + noise2D( tx - 1	, ty + 1 ) + noise2D( tx + 1, ty + 1 ) ) / 16;
	Float sides   = ( noise2D( tx - 1, ty		) + noise2D( tx + 1, ty		) + noise2D( tx		, ty - 1 ) + noise2D( tx	, ty + 1 ) ) /  8;
	Float center  = noise2D( tx, ty ) / 4;

	return corners + sides + center;
}

Float PerlinNoise::interpolate( Float a, Float b, Float x ) {
	Float fac1 = 3 * eepow( 1 - x, 2	) - 2 * eepow( 1 - x, 3 );
	Float fac2 = 3 * eepow( x, 2		) - 2 * eepow( x, 3		);

	return a * fac1 + b * fac2; //add the weighted factors
}

Float PerlinNoise::interpolatedNoise2D(Float x, Float y) {
	Int32 eger_X = static_cast<Int32>( x );
	Int32 eger_Y = static_cast<Int32>( y );

	Float fractional_X = x - eger_X;
	Float fractional_Y = y - eger_Y;

	Float feger_X = static_cast<Float> ( eger_X );
	Float feger_Y = static_cast<Float> ( eger_Y );

	Float v1 = smoothedNoise2D( feger_X		, feger_Y		);
	Float v2 = smoothedNoise2D( feger_X + 1.f	, feger_Y		);
	Float v3 = smoothedNoise2D( feger_X		, feger_Y + 1.f	);
	Float v4 = smoothedNoise2D( feger_X + 1.f	, feger_Y + 1.f	);

	Float i1 = interpolate( v1 , v2 , fractional_X );
	Float i2 = interpolate( v3 , v4 , fractional_X );

	return interpolate( i1 , i2 , fractional_Y );
}

}}
