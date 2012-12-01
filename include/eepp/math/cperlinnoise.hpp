#ifndef EE_MATHCPERLINNOISE_H
#define EE_MATHCPERLINNOISE_H
/*
My code is based on this sites:
http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
http://www.animeimaging.com/asp/PerlinNoise.aspx - by James Long
And for the C++ implementation of Henrik Krysell.
*/

#include <eepp/declares.hpp>

namespace EE { namespace Math {

class EE_API cPerlinNoise {
	public:
		cPerlinNoise();
		
		~cPerlinNoise();

		void Init();

		eeFloat PerlinNoise2D(eeFloat x, eeFloat y);

		void Octaves( const eeInt& octaves ) { mOctaves = octaves; }
		
		void Persistence( const eeFloat& pers)  { mPersistence = pers; }
		
		void Frequency( const eeFloat& freq ) { mFrequency = freq; }
		
		void Amplitude( const eeFloat& amp ) { mAmplitude = amp; }
		
		void FrequencyOctaveDep( const bool& dep ) { mFreqOctaveDep =  dep; }
		
		void AmplitudeOctaveDep( const bool& dep ) { mAmpOctaveDep = dep; }

		eeInt Octaves() const { return mOctaves; }
		
		eeFloat Persistence() const { return mPersistence; }
		
		eeFloat Frequency() const { return mFrequency; }
		
		eeFloat Amplitude() const { return mAmplitude; }
		
		bool FrequencyOctaveDep() const { return mFreqOctaveDep; }
		
		bool AmplitudeOctaveDep() const { return mAmpOctaveDep; }
	protected:
		eeFloat Noise2D(Int32 x, Int32 y);
		
		eeFloat SmoothedNoise2D(eeFloat x, eeFloat y);
		
		eeFloat Interpolate(eeFloat a, eeFloat b, eeFloat x);
		
		eeFloat InterpolatedNoise2D(eeFloat x, eeFloat y);

		eeFloat	mCurrSeed;
		eeFloat	mPersistence;
		eeInt	mOctaves;
		eeFloat	mFrequency;
		eeFloat	mAmplitude;

		bool	mFreqOctaveDep;
		bool	mAmpOctaveDep;
};

}}
#endif
