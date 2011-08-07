#ifndef EECPARTICLESYSTEM_H
#define EECPARTICLESYSTEM_H

#include "base.hpp"
#include "cparticle.hpp"

namespace EE { namespace Graphics {

/** @enum EE_PARTICLE_EFFECT Predefined effects for the particle system. Use Callback when wan't to create a new effect, o set the parameters using NoFx, but it's much more limited. */
enum EE_PARTICLE_EFFECT {
	PSE_Nofx = 0, //!< User defined effect
	PSE_BlueBall,
	PSE_Fire,
	PSE_Smoke,
	PSE_Snow,
	PSE_MagicFire,
	PSE_LevelUp,
	PSE_LevelUp2,
	PSE_Heal,
	PSE_WormHole,
	PSE_Twirl,
	PSE_Flower,
	PSE_Galaxy,
	PSE_Heart,
	PSE_BlueExplosion,
	PSE_GP,
	PSE_BTwirl,
	PSE_BT,
	PSE_Atomic,
	PSE_Callback //!< Callback defined effect. Set the callback before creating the effect.
};

/** @brief Basic but powerfull Particle System */
class EE_API cParticleSystem {
	public:
		typedef cb::Callback2<void, cParticle*, cParticleSystem*> ParticleCallback;

		cParticleSystem();

		virtual ~cParticleSystem();

		/** Creates the new effect
		* @param Effect Number of the effect.
		* @param NumParticles Number of particles
		* @param TexId Texture Id to render the particles
		* @param Pos Initial position
		* @param PartSize Size of the particles
		* @param AnimLoop Loop the animation?
		* @param NumLoops If AnimLoop is false, set the number of times to render the effect
		* @param Color Particles Color (used for NoFx)
		* @param Pos2 Extended position for some effects to define the expansion over the screen (used by fire,smoke,snow)
		* @param AlphaDecay The Alpha Decay for the particles (used for NoFx)
		* @param Speed The speed on x axis (used for NoFx)
		* @param Acc The acceleration of the particle (used for NoFx)
		*/
		void Create(
			const EE_PARTICLE_EFFECT& Effect,
			const Uint32& NumParticles,
			const Uint32& TexId,
			const eeVector2f& Pos,
			const eeFloat& PartSize = 16.0f,
			const bool& AnimLoop = false,
			const Uint32& NumLoops = 1,
			const eeColorAf& Color = eeColorAf( 1.0f, 1.0f, 1.0f, 1.0f ),
			const eeVector2f& Pos2 = eeVector2f( 0, 0 ),
			const eeFloat& AlphaDecay = 0.01f,
			const eeVector2f& Speed = eeVector2f( 0.1f, 0.1f ),
			const eeVector2f& Acc = eeVector2f( 0.1f, 0.1f )
		);

		/** Draw the particles effect */
		void Draw();

		/** Update the particles effect
		* @param Time The time transcurred between the last update.
		*/
		void Update( const eeFloat& Time );

		/** Update the particles effect taking the elapsed time from cEngine */
		void Update();

		/** Stop using the effect but wait to end the animation */
		void End();

		/** Start using the effect and reset it */
		void ReUse();

		/** Stop immediately the effect */
		void Kill();

		/** Change the default time modifier. Default 0.01f */
		void Time( const eeFloat& time );

		/** Get the time modifier */
		eeFloat Time() const;

		/** Set if the effect it's in use */
		void Using( const bool& inuse );

		/** @return It's used or no */
		bool Using() const;

		/** Update the effect position */
		void Position( const eeFloat& x, const eeFloat& y );

		/** Update the effect position */
		void Position( const eeVector2f& Pos );

		/** @return The effect position */
		const eeVector2f& Position() const;

		/** Update the effect position 2 */
		void Position2( const eeFloat& x, const eeFloat& y );

		/** Update the effect position 2 */
		void Position2( const eeVector2f& Pos );

		/** @return The effect position 2 */
		const eeVector2f& Position2() const;

		/** Set a callback function for the reset effect of the particles. \n The reset it's where do you create the effect for every single particle. */
		void SetCallbackReset( const ParticleCallback& pc );

		/** @return The effect blend mode */
		const EE_PRE_BLEND_FUNC& BlendMode() const;

		/** Set the effect blend mode */
		void BlendMode( const EE_PRE_BLEND_FUNC& mode );

		/** @return The color of the effect */
		const eeColorAf& Color() const;

		/** Set the color of the effect */
		void Color( const eeColorAf& Col );

		/** @return The alpha decay of the effect */
		const eeFloat& AlphaDecay() const;

		/** Set the alpha decay of the effect */
		void AlphaDecay( const eeFloat& Decay );

		/** @return The Speed of the effect */
		const eeVector2f& Speed() const;

		/** Set the Speed of the effect */
		void Speed( const eeVector2f& speed );

		/** @return The Acceleration of the effect */
		const eeVector2f& Acceleration() const;

		/** Set The Acceleration of the effect */
		void Acceleration( const eeVector2f& acc );
	private:
		cParticle *			mParticle;
		Uint32				mPCount;
		Uint32				mTexId;
		Uint32				mPLeft;
		Uint32				mLoops;

		EE_PARTICLE_EFFECT	mEffect;
		EE_PRE_BLEND_FUNC	mBlend;

		eeColorAf			mColor;

		eeInt				mProgression;
		eeInt				mDirection;

		eeVector2f			mPos;
		eeVector2f			mPos2;
		eeVector2f			mAcc;
		eeVector2f			mSpeed;
		eeFloat				mAlphaDecay;
		eeFloat				mSize;
		eeFloat				mHSize;
		eeFloat				mTime;

		bool				mLoop;
		bool				mUsed;
		bool				mPointsSup;

		void Begin();

		virtual void Reset( cParticle * P );

		ParticleCallback mPC;
};

}}

#endif
