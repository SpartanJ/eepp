#ifndef EECPARTICLESYSTEM_H
#define EECPARTICLESYSTEM_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/particle.hpp>

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
class EE_API ParticleSystem {
	public:
		typedef cb::Callback2<void, Particle*, ParticleSystem*> ParticleCallback;

		ParticleSystem();

		virtual ~ParticleSystem();

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
		void create(
			const EE_PARTICLE_EFFECT& Effect,
			const Uint32& NumParticles,
			const Uint32& TexId,
			const Vector2f& Pos,
			const Float& PartSize = 16.0f,
			const bool& AnimLoop = false,
			const Uint32& NumLoops = 1,
			const ColorAf& color = ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ),
			const Vector2f& Pos2 = Vector2f( 0, 0 ),
			const Float& alphaDecay = 0.01f,
			const Vector2f& speed = Vector2f( 0.1f, 0.1f ),
			const Vector2f& Acc = Vector2f( 0.1f, 0.1f )
		);

		/** Draw the particles effect */
		void draw();

		/** Update the particles effect
		* @param Time The time transcurred between the last update.
		*/
		void update(const Time& time);

		/** Update the particles effect taking the elapsed time from Engine */
		void update();

		/** Stop using the effect but wait to end the animation */
		void end();

		/** Start using the effect and reset it */
		void reuse();

		/** Stop immediately the effect */
		void kill();

		/** Change the default time modifier. Default 0.01f */
		void time( const Float& time );

		/** Get the time modifier */
		Float time() const;

		/** Set if the effect it's in use */
		void setUsing( const bool& inuse );

		/** @return It's used or no */
		bool isUsing() const;

		/** Update the effect position */
		void setPosition( const Float& x, const Float& y );

		/** Update the effect position */
		void setPosition( const Vector2f& Pos );

		/** @return The effect position */
		const Vector2f& getPosition() const;

		/** Update the effect position 2 */
		void setPosition2( const Float& x, const Float& y );

		/** Update the effect position 2 */
		void setPosition2( const Vector2f& Pos );

		/** @return The effect position 2 */
		const Vector2f& getPosition2() const;

		/** Set a callback function for the reset effect of the particles. \n The reset it's where do you create the effect for every single particle. */
		void setCallbackReset( const ParticleCallback& pc );

		/** @return The effect blend mode */
		const BlendMode& getBlendMode() const;

		/** Set the effect blend mode */
		void setBlendMode( const BlendMode& mode );

		/** @return The color of the effect */
		const ColorAf& getColor() const;

		/** Set the color of the effect */
		void setColor( const ColorAf& Col );

		/** @return The alpha decay of the effect */
		const Float& getAlphaDecay() const;

		/** Set the alpha decay of the effect */
		void setAlphaDecay( const Float& Decay );

		/** @return The Speed of the effect */
		const Vector2f& getSpeed() const;

		/** Set the Speed of the effect */
		void setSpeed( const Vector2f& speed );

		/** @return The Acceleration of the effect */
		const Vector2f& getAcceleration() const;

		/** Set The Acceleration of the effect */
		void setAcceleration( const Vector2f& acc );
	private:
		Particle *			mParticle;
		Uint32				mPCount;
		Uint32				mTexId;
		Uint32				mPLeft;
		Uint32				mLoops;

		EE_PARTICLE_EFFECT	mEffect;
		BlendMode	mBlend;

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

		void begin();

		virtual void reset( Particle * P );

		ParticleCallback mPC;
};

}}

#endif
