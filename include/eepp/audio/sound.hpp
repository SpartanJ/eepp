#ifndef EE_AUDIOCSOUND_H
#define EE_AUDIOCSOUND_H

#include <eepp/audio/base.hpp>
#include <eepp/audio/audiolistener.hpp>
#include <eepp/audio/soundbuffer.hpp>

namespace EE { namespace Audio {

/** @brief Regular sound that can be played in the audio environment */
class EE_API Sound {
	public :
		/** @brief The state of the sound */
		enum Status {
			Stopped,
			Paused,
			Playing
		};

		Sound();

		~Sound();

		/** Construct the sound with a buffer. */
		Sound( const SoundBuffer& Buffer, const bool& Loop = false, const float& Pitch = 1.f, const float& Volume = 100.f, const eeVector3ff& Position = eeVector3ff(0, 0, 0) );

		/** Copy constructor */
		Sound(const Sound& Copy);

		/** Play the Sound */
		void Play();

		/** Pause the Sound */
		void Pause();

		/** Stop the Sound */
		void Stop();

		/** Set the Sound Source Buffer */
		void Buffer( const SoundBuffer& Buffer );

		/** Set the Sound Loop State */
		void Loop( const bool& Loop );

		/** Set the Sound Pitch */
		void Pitch( const float& Pitch );

		/** Set the Sound Volume */
		void Volume( const float& Volume );

		/** Set the Sound Position. The default position is (0, 0, 0) */
		void Position( const float& X, const float& Y, const float& Z );

		/** Set the Sound Position from a 3D Vector. The default position is (0, 0, 0) */
		void Position( const eeVector3ff& Position );

		/** Set the minimum distance - closer than this distance, \n the listener will hear the sound at its maximum volume. \n The default minimum distance is 1.0. */
		void MinDistance( const float& MinDistance );

		/** Set the attenuation factor. \n The higher the attenuation, the more the sound will be attenuated with distance from listener. \n The default attenuation factor 1.0. */
		void Attenuation( const float& Attenuation );

		/** Get the Sound Source Buffer */
		const SoundBuffer* Buffer() const;

		/** Get the Sound Loop State */
		bool Loop() const;

		/** Get the Sound Pitch */
		float Pitch() const;

		/** Get the Sound Volume */
		float Volume() const;

		/** Get the Sound Position */
		eeVector3ff Position() const;

		/** Get the Minimun Distance */
		float MinDistance() const;

		/** Get the Sound Attenuation */
		float Attenuation() const;

		/** Get the Sound State */
		Status GetState() const;

		/** Get the Sound State */
		Status State() const;

		/** Get the current playing position of the sound */
		virtual Time PlayingOffset() const;

		/** Set the current playing position of the sound
		* @param TimeOffset : New playing position
		*/
		virtual void PlayingOffset( const Time &TimeOffset );

		/** Assignment operator */
		Sound& operator =(const Sound& Other);

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
		* to use it. It is called by the SoundBuffer that
		* this sound uses, when it is destroyed in order to prevent
		* the sound from using a dead buffer. */
		void ResetBuffer();
	private :
		friend class SoundStream;

		unsigned int			mSource; 	///< OpenAL source identifier
		const SoundBuffer *	mBuffer; 	///< Sound buffer bound to the source
};

}}

#endif
