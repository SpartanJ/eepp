#ifndef EE_AUDIOCSOUND_H
#define EE_AUDIOCSOUND_H

#include <eepp/audio/base.hpp>
#include <eepp/audio/caudiolistener.hpp>
#include <eepp/audio/csoundbuffer.hpp>

namespace EE { namespace Audio {

/** @brief Regular sound that can be played in the audio environment */
class EE_API cSound {
	public :
		/** @enum cSound::Status The state of the sound */
		enum Status {
			Stopped,
			Paused,
			Playing
		};

		cSound();

		~cSound();

		/** Construct the sound with a buffer. */
		cSound( const cSoundBuffer& Buffer, const bool& Loop = false, const eeFloat& Pitch = 1.f, const eeFloat& Volume = 100.f, const Vector3AL& Position = Vector3AL(0, 0, 0) );

		/** Copy constructor */
		cSound(const cSound& Copy);

		/** Play the Sound */
		void Play();

		/** Pause the Sound */
		void Pause();

		/** Stop the Sound */
		void Stop();

		/** Set the Sound Source Buffer */
		void Buffer( const cSoundBuffer& Buffer );

		/** Set the Sound Loop State */
		void Loop( const bool& Loop );

		/** Set the Sound Pitch */
		void Pitch( const eeFloat& Pitch );

		/** Set the Sound Volume */
		void Volume( const eeFloat& Volume );

		/** Set the Sound Position. The default position is (0, 0, 0) */
		void Position( const eeFloat& X, const eeFloat& Y, const eeFloat& Z );

		/** Set the Sound Position from a 3D Vector. The default position is (0, 0, 0) */
		void Position( const Vector3AL& Position );

		/** Set the minimum distance - closer than this distance, \n the listener will hear the sound at its maximum volume. \n The default minimum distance is 1.0. */
		void MinDistance( const eeFloat& MinDistance );

		/** Set the attenuation factor. \n The higher the attenuation, the more the sound will be attenuated with distance from listener. \n The default attenuation factor 1.0. */
		void Attenuation( const eeFloat& Attenuation );

		/** Get the Sound Source Buffer */
		const cSoundBuffer* Buffer() const;

		/** Get the Sound Loop State */
		bool Loop() const;

		/** Get the Sound Pitch */
		eeFloat Pitch() const;

		/** Get the Sound Volume */
		eeFloat Volume() const;

		/** Get the Sound Position */
		Vector3AL Position() const;

		/** Get the Minimun Distance */
		eeFloat MinDistance() const;

		/** Get the Sound Attenuation */
		eeFloat Attenuation() const;

		/** Get the Sound State */
		Status GetState() const;

		/** Get the Sound State */
		Status State() const;

		/** Get the current playing position of the sound */
		virtual Uint32 PlayingOffset() const;

		/** Set the current playing position of the sound
		* @param TimeOffset : New playing position, expressed in miliseconds
		*/
		virtual void PlayingOffset( const Uint32& TimeOffset );

		/** Assignment operator */
		cSound& operator =(const cSound& Other);

		/** Make the sound's position relative to the listener's position, or absolute. The default value is false (absolute)
		* @param Relative : True to set the position relative, false to set it absolute
		*/
		void SetRelativeToListener( const bool& Relative );

		/** Tell if the sound's position is relative to the listener's position, or if it's absolute
		* @return True if the position is relative, false if it's absolute
		*/
		bool IsRelativeToListener() const;

		/** \brief Reset the internal buffer of the sound
		* This function is for internal use only, you don't have
		* to use it. It is called by the cSoundBuffer that
		* this sound uses, when it is destroyed in order to prevent
		* the sound from using a dead buffer. */
		void ResetBuffer();
	private :
		friend class cSoundStream;

		unsigned int			mSource; 	///< OpenAL source identifier
		const cSoundBuffer *	mBuffer; 	///< Sound buffer bound to the source
};

}}

#endif
