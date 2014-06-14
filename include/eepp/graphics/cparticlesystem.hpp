#ifndef EECPARTICLESYSTEM_H
#define EECPARTICLESYSTEM_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cparticle.hpp>

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
			const Vector2f& Pos,
			const Float& PartSize = 16.0f,
			const bool& AnimLoop = false,
			const Uint32& NumLoops = 1,
			const ColorAf& Color = ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ),
			const Vector2f& Pos2 = Vector2f( 0, 0 ),
			const Float& AlphaDecay = 0.01f,
			const Vector2f& Speed = Vector2f( 0.1f, 0.1f ),
			const Vector2f& Acc = Vector2f( 0.1f, 0.1f )
		);

		/** Draw the particles effect */
		void Draw();

		/** Update the particles effect
		* @param Time The time transcurred between the last update.
		*/
		void Update(const Time &time );

		/** Update the particles effect taking the elapsed time from Engine */
		void Update();

		/** Stop using the effect but wait to end the animation */
		void End();

		/** Start using the effect and reset it */
		void ReUse();

		/** Stop immediately the effect */
		void Kill();

		/** Change the default time modifier. Default 0.01f */
		void Time( const Float& time );

		/** Get the time modifier */
		Float Time() const;

		/** Set if the effect it's in use */
		void Using( const bool& inuse );

		/** @return It's used or no */
		bool Using() const;

		/** Update the effect position */
		void Position( const Float& x, const Float& y );

		/** Update the effect position */
		void Position( const Vector2f& Pos );

		/** @return The effect position */
		const Vector2f& Position() const;

		/** Update the effect position 2 */
		void Position2( const Float& x, const Float& y );

		/** Update the effect position 2 */
		void Position2( const Vector2f& Pos );

		/** @return The effect position 2 */
		const Vector2f& Position2() const;

		/** Set a callback function for the reset effect of the particles. \n The reset it's where do you create the effect for every single particle. */
		void SetCallbackReset( const ParticleCallback& pc );

		/** @return The effect blend mode */
		const EE_BLEND_MODE& BlendMode() const;

		/** Set the effect blend mode */
		void BlendMode( const EE_BLEND_MODE& mode );

		/** @return The color of the effect */
		const ColorAf& Color() const;

		/** Set the color of the effect */
		void Color( const ColorAf& Col );

		/** @return The alpha decay of the effect */
		const Float& AlphaDecay() const;

		/** Set the alpha decay of the effect */
		void AlphaDecay( const Float& Decay );

		/** @return The Speed of the effect */
		const Vector2f& Speed() const;

		/** Set the Speed of the effect */
		void Speed( const Vector2f& speed );

		/** @return The Acceleration of the effect */
		const Vector2f& Acceleration() const;

		/** Set The Acceleration of the effect */
		void Acceleration( const Vector2f& acc );
	private:
		cParticle *			mParticle;
		Uint32				mPCount;
		Uint32				mTexId;
		Uint32				mPLeft;
		Uint32				mLoops;

		EE_PARTICLE_EFFECT	mEffect;
		EE_BLEND_MODE	mBlend;

		ColorAf			mColor;

		int				mProgression;
		int				mDirection;

		Vector2f			mPos;
		Vector2f			mPos2;
		Vector2f			mAcc;
		Vector2f			mSpeed;
		Float				mAlphaDecay;
		Float				mSize;
		Float				mHSize;
		Float				mTime;

		bool				mLoop;
		bool				mUsed;
		bool				mPointsSup;

		void Begin();

		virtual void Reset( cParticle * P );

		ParticleCallback mPC;
};

}}

#endif
