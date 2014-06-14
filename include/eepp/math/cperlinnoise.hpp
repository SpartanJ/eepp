#ifndef EE_MATHCPERLINNOISE_H
#define EE_MATHCPERLINNOISE_H
/*
My code is based on this sites:
http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
http://www.animeimaging.com/asp/PerlinNoise.aspx - by James Long
And for the C++ implementation of Henrik Krysell.
*/

#include <eepp/config.hpp>

namespace EE { namespace Math {

/** @brief Perlin noise can be used to generate gradients, textures and effects. For more information go to http://en.wikipedia.org/wiki/Perlin_noise
**	To get a better understanding of the variables to generate the noise, visit http://freespace.virgin.net/hugo.elias/models/m_perlin.htm */
class EE_API cPerlinNoise {
	public:
		cPerlinNoise();
		
		~cPerlinNoise();

		/** Reset the initial values */
		void Init();

		/** @return The noise value for the 2D coordinates */
		Float PerlinNoise2D(Float x, Float y);

		void Octaves( const int& octaves ) { mOctaves = octaves; }
		
		void Persistence( const Float& pers)  { mPersistence = pers; }

		void Frequency( const Float& freq ) { mFrequency = freq; }

		void Amplitude( const Float& amp ) { mAmplitude = amp; }
		
		void FrequencyOctaveDep( const bool& dep ) { mFreqOctaveDep =  dep; }
		
		void AmplitudeOctaveDep( const bool& dep ) { mAmpOctaveDep = dep; }

		int Octaves() const { return mOctaves; }
		
		Float Persistence() const { return mPersistence; }
		
		Float Frequency() const { return mFrequency; }
		
		Float Amplitude() const { return mAmplitude; }
		
		bool FrequencyOctaveDep() const { return mFreqOctaveDep; }
		
		bool AmplitudeOctaveDep() const { return mAmpOctaveDep; }
	protected:
		Float Noise2D(Int32 x, Int32 y);
		
		Float SmoothedNoise2D(Float x, Float y);
		
		Float Interpolate(Float a, Float b, Float x);
		
		Float InterpolatedNoise2D(Float x, Float y);

		Float	mCurrSeed;
		Float	mPersistence;
		int	mOctaves;
		Float	mFrequency;
		Float	mAmplitude;

		bool	mFreqOctaveDep;
		bool	mAmpOctaveDep;
};

}}
#endif
