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
		void setBuffer( const SoundBuffer& buffer );

		/** Get the Sound Source Buffer */
		const SoundBuffer* getBuffer() const;

		/** Set the Sound Loop State */
		void setLoop( const bool& loop );

		/** Get the Sound Loop State */
		bool getLoop() const;

		/** Set the Sound Pitch */
		void setPitch( const float& pitch );

		/** Get the Sound Pitch */
		float getPitch() const;

		/** Set the Sound Volume */
		void setVolume( const float& volume );

		/** Get the Sound Volume */
		float getVolume() const;

		/** Set the Sound Position. The default position is (0, 0, 0) */
		void setPosition( const float& X, const float& Y, const float& Z );

		/** Set the Sound Position from a 3D Vector. The default position is (0, 0, 0) */
		void setPosition( const Vector3ff& position );

		/** Get the Sound Position */
		Vector3ff getPosition() const;

		/** Set the minimum distance - closer than this distance, \n the listener will hear the sound at its maximum volume. \n The default minimum distance is 1.0. */
		void setMinDistance( const float& minDistance );

		/** Get the Minimun Distance */
		float getMinDistance() const;

		/** Set the attenuation factor. \n The higher the attenuation, the more the sound will be attenuated with distance from listener. \n The default attenuation factor 1.0. */
		void setAttenuation( const float& attenuation );

		/** Get the Sound Attenuation */
		float getAttenuation() const;

		/** Get the Sound State */
		Status getState() const;

		/** Get the current playing position of the sound */
		virtual Time getPlayingOffset() const;

		/** Set the current playing position of the sound
		* @param TimeOffset : New playing position
		*/
		virtual void setPlayingOffset( const Time &TimeOffset );

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

		unsigned int mSource; 	///< OpenAL source identifier
		const SoundBuffer *	mBuffer; 	///< Sound buffer bound to the source
};

}}

#endif
