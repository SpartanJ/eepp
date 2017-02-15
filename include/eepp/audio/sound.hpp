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
		Sound( const SoundBuffer& buffer, const bool& loop = false, const float& pitch = 1.f, const float& volume = 100.f, const Vector3ff& position = Vector3ff(0, 0, 0) );

		/** Copy constructor */
		Sound(const Sound& Copy);

		/** Play the Sound */
		void play();

		/** Pause the Sound */
		void pause();

		/** Stop the Sound */
		void stop();

		/** Set the Sound Source Buffer */
		void buffer( const SoundBuffer& buffer );

		/** Set the Sound Loop State */
		void loop( const bool& loop );

		/** Set the Sound Pitch */
		void pitch( const float& pitch );

		/** Set the Sound Volume */
		void volume( const float& volume );

		/** Set the Sound Position. The default position is (0, 0, 0) */
		void position( const float& X, const float& Y, const float& Z );

		/** Set the Sound Position from a 3D Vector. The default position is (0, 0, 0) */
		void position( const Vector3ff& position );

		/** Set the minimum distance - closer than this distance, \n the listener will hear the sound at its maximum volume. \n The default minimum distance is 1.0. */
		void minDistance( const float& minDistance );

		/** Set the attenuation factor. \n The higher the attenuation, the more the sound will be attenuated with distance from listener. \n The default attenuation factor 1.0. */
		void attenuation( const float& attenuation );

		/** Get the Sound Source Buffer */
		const SoundBuffer* buffer() const;

		/** Get the Sound Loop State */
		bool loop() const;

		/** Get the Sound Pitch */
		float pitch() const;

		/** Get the Sound Volume */
		float volume() const;

		/** Get the Sound Position */
		Vector3ff position() const;

		/** Get the Minimun Distance */
		float minDistance() const;

		/** Get the Sound Attenuation */
		float attenuation() const;

		/** Get the Sound State */
		Status getState() const;

		/** Get the Sound State */
		Status state() const;

		/** Get the current playing position of the sound */
		virtual Time playingOffset() const;

		/** Set the current playing position of the sound
		* @param TimeOffset : New playing position
		*/
		virtual void playingOffset( const Time &TimeOffset );

		/** Assignment operator */
		Sound& operator =(const Sound& Other);

		/** Make the sound's position relative to the listener's position, or absolute. The default value is false (absolute)
		* @param Relative : True to set the position relative, false to set it absolute
		*/
		void setRelativeToListener( const bool& Relative );

		/** Tell if the sound's position is relative to the listener's position, or if it's absolute
		* @return True if the position is relative, false if it's absolute
		*/
		bool isRelativeToListener() const;

		/** \brief Reset the internal buffer of the sound
		* This function is for internal use only, you don't have
		* to use it. It is called by the SoundBuffer that
		* this sound uses, when it is destroyed in order to prevent
		* the sound from using a dead buffer. */
		void resetBuffer();
	private :
		friend class SoundStream;

		unsigned int			mSource; 	///< OpenAL source identifier
		const SoundBuffer *	mBuffer; 	///< Sound buffer bound to the source
};

}}

#endif
