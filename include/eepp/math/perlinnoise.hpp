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

/** @brief Perlin noise can be used to generate gradients, textures and effects. For more
 *information go to http://en.wikipedia.org/wiki/Perlin_noise *	To get a better understanding of
 *the variables to generate the noise, visit
 *http://freespace.virgin.net/hugo.elias/models/m_perlin.htm */
class EE_API PerlinNoise {
  public:
	PerlinNoise();

	~PerlinNoise();

	/** Reset the initial values */
	void init();

	/** @return The noise value for the 2D coordinates */
	Float getPerlinNoise2D( Float x, Float y );

	void setOctaves( const int& octaves ) { mOctaves = octaves; }

	void setPersistence( const Float& pers ) { mPersistence = pers; }

	void setFrequency( const Float& freq ) { mFrequency = freq; }

	void setAmplitude( const Float& amp ) { mAmplitude = amp; }

	void setFrequencyOctaveDep( const bool& dep ) { mFreqOctaveDep = dep; }

	void setAmplitudeOctaveDep( const bool& dep ) { mAmpOctaveDep = dep; }

	int getOctaves() const { return mOctaves; }

	Float getPersistence() const { return mPersistence; }

	Float getFrequency() const { return mFrequency; }

	Float getAmplitude() const { return mAmplitude; }

	bool getFrequencyOctaveDep() const { return mFreqOctaveDep; }

	bool getAmplitudeOctaveDep() const { return mAmpOctaveDep; }

  protected:
	Float noise2D( Int32 x, Int32 y );

	Float smoothedNoise2D( Float x, Float y );

	Float interpolate( Float a, Float b, Float x );

	Float interpolatedNoise2D( Float x, Float y );

	Float mCurrSeed;
	Float mPersistence;
	int mOctaves;
	Float mFrequency;
	Float mAmplitude;

	bool mFreqOctaveDep;
	bool mAmpOctaveDep;
};

}} // namespace EE::Math
#endif
