#ifndef EECPARTICLESYSTEM_H
#define EECPARTICLESYSTEM_H

#include "base.hpp"
#include "cparticle.hpp"
#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"

using namespace EE::Window;

namespace EE { namespace Graphics {

/** @enum EE_PARTICLE_EFFECT Predefined effects for the particle system. Use Callback when wan't to create a new effect, o set the parameters using NoFx, but it's much more limited. */
typedef enum {
	Nofx = 0, //!< User defined effect
	BlueBall = 1,
	Fire = 2,
	Smoke = 3,
	Snow = 4,
	MagicFire = 5,
	LevelUp = 6,
	LevelUp2 = 7,
	Heal = 8,
	WormHole = 9,
	Twirl = 10,
	Flower = 11,
	Galaxy = 12,
	Heart = 13,
	BlueExplosion = 14,
	GP = 15,
	BTwirl = 16,
	BT = 17,
	Atomic = 18,
	Callback = 19 //!< Callback defined effect. Set the callback before creating the effect.
} EE_PARTICLE_EFFECT;

/** @brief Basic but powerfull Particle System */
class EE_API cParticleSystem {
	public:
		typedef boost::function2<void, cParticle*, cParticleSystem*> ParticleCallback;
		
		cParticleSystem();
		~cParticleSystem();

		/** Creates the new effect
		* @param Effect Number of the effect.
		* @param NumParticles Number of particles
		* @param TexId Texture Id to render the particles
		* @param X Initial x position
		* @param Y Initial y position
		* @param PartSize Size of the particles
		* @param AnimLoop Loop the animation?
		* @param NumLoops If AnimLoop is false, set the number of times to render the effect
		* @param Color Particles Color
		* @param X2 Extended x position for some effects (used for NoFx)
		* @param Y2 Extended y position for some effects (used for NoFx)
		* @param AlphaDecay The Alpha Decay for the particles (used for NoFx)
		* @param XSpeed The speed on x axis (used for NoFx)
		* @param YSpeed The speed on y axis (used for NoFx)
		* @param XAcceleration The acceleration of the particle on x axis (used for NoFx)
		* @param YAcceleration The acceleration of the particle on x axis (used for NoFx)
		*/
		void Create(const EE_PARTICLE_EFFECT& Effect, const Uint32& NumParticles, const Uint32& TexId, const eeFloat& X = 0, const eeFloat& Y = 0, const eeFloat& PartSize = 16.0f, const bool& AnimLoop = false, const Uint32& NumLoops = 1, const eeColorAf& Color = eeColorAf(1.0f,1.0f,1.0f,1.0f), const eeFloat& X2 = 0, const eeFloat& Y2 = 0, const eeFloat& AlphaDecay = 0.01f, const eeFloat& XSpeed = 0.1f, const eeFloat& YSpeed = 0.1f, const eeFloat& XAcceleration = 0.1f, const eeFloat& YAcceleration = 0.1f);

		/** Draw the particles effect */
		void Draw();

		/** Update the particles effect 
		* @param Time The time transcurred between the last update. If -1 will take the cEngine::Elapsed()
		*/
		void Update( const eeFloat& Time = -99999.f );

		/** Stop using the effect but wait to end the animation */
		void End();

		/** Start using the effect and reset it */
		void ReUse();

		/** Stop immediately the effect */
		void Kill();

		/** Draw and Update the effect. Don't call Draw or Update if you use this. */
		void DrawUpdate();

		/** Change the default time modifier. Default 0.01f */
		void Time(const eeFloat& time) { if (time>0) mTime = time; }

		/** Get the time modifier */
		eeFloat Time() const { return mTime; }

		/** Set if the effect it's in use */
		void Using(const bool& inuse) { mUsed = inuse; }

		/** @return It's used or no */
		bool Using() const { return mUsed; }

		/** Set the x axis position */
		void X(const eeFloat& x) { mX = x; }

		/** Get the x axis position */
		eeFloat X() const { return mX; }

		/** Set the y axis position */
		void Y(const eeFloat& y) { mY = y; }

		/** Get the y axis position */
		eeFloat Y() const { return mY; }

		/** Set the x2 axis position */
		void X2(const eeFloat& x2) { mX2 = x2; mX2 = x2 + (mX2 - mX); }

		/** Get the x2 axis position */
		eeFloat X2() const { return mX2; }

		/** Set the y2 axis position */
		void Y2(const eeFloat& y2) { mY2 = y2; mY2 = y2 + (mY2 - mY); }

		/** Get the y2 axis position */
		eeFloat Y2() const { return mY2; }

		/** Update the effect position */
		void UpdatePos(const eeFloat& x, const eeFloat& y);

		/** Set a callback function for the reset effect of the particles. \n The reset it's where do you create the effect for every single particle. */
		void SetCallbackReset( const ParticleCallback& pc );
	private:
		cEngine* EE;
		cTextureFactory* TF;
		
		std::vector<cParticle> mParticle;
		EE_PARTICLE_EFFECT mEffect;
		eeColorAf mColor;
		
		Uint32 mTexId, mPCount, mPLeft, mLoops;
		int mProgression, mDirection;
		bool mLoop, mUsed, mPointsSup;
		
		eeFloat mX, mY, mXAcc, mYAcc, mXSpeed, mYSpeed, mAlphaDecay, mSize, mHSize, mTime, mX2, mY2;
		
		void Begin();
		void Reset( cParticle* P );
		
		bool mIsCallback;
		ParticleCallback mPC;
};

}}

#endif
