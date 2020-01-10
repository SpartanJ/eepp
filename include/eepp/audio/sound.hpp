#ifndef EE_AUDIO_SOUND_HPP
#define EE_AUDIO_SOUND_HPP

#include <cstdlib>
#include <eepp/audio/soundsource.hpp>
#include <eepp/config.hpp>
#include <eepp/system/time.hpp>
using namespace EE::System;

namespace EE { namespace Audio {

class SoundBuffer;

/// \brief Regular sound that can be played in the audio environment
class EE_API Sound : public SoundSource {
  public:
	Sound();

	/// \brief Construct the sound with a buffer
	/// \param buffer Sound buffer containing the audio data to play with the sound
	explicit Sound( const SoundBuffer& buffer );

	/// \brief Copy constructor
	/// \param copy Instance to copy
	Sound( const Sound& copy );

	~Sound();

	////////////////////////////////////////////////////////////
	/// \brief Start or resume playing the sound
	///
	/// This function starts the stream if it was stopped, resumes
	/// it if it was paused, and restarts it from beginning if it
	/// was it already playing.
	/// This function uses its own thread so that it doesn't block
	/// the rest of the program while the sound is played.
	///
	/// \see pause, stop
	///
	////////////////////////////////////////////////////////////
	void play();

	////////////////////////////////////////////////////////////
	/// \brief Pause the sound
	///
	/// This function pauses the sound if it was playing,
	/// otherwise (sound already paused or stopped) it has no effect.
	///
	/// \see play, stop
	///
	////////////////////////////////////////////////////////////
	void pause();

	////////////////////////////////////////////////////////////
	/// \brief stop playing the sound
	///
	/// This function stops the sound if it was playing or paused,
	/// and does nothing if it was already stopped.
	/// It also resets the playing position (unlike pause()).
	///
	/// \see play, pause
	///
	////////////////////////////////////////////////////////////
	void stop();

	////////////////////////////////////////////////////////////
	/// \brief Set the source buffer containing the audio data to play
	///
	/// It is important to note that the sound buffer is not copied,
	/// thus the SoundBuffer instance must remain alive as long
	/// as it is attached to the sound.
	///
	/// \param buffer Sound buffer to attach to the sound
	///
	/// \see getBuffer
	///
	////////////////////////////////////////////////////////////
	void setBuffer( const SoundBuffer& buffer );

	////////////////////////////////////////////////////////////
	/// \brief Set whether or not the sound should loop after reaching the end
	///
	/// If set, the sound will restart from beginning after
	/// reaching the end and so on, until it is stopped or
	/// setLoop(false) is called.
	/// The default looping state for sound is false.
	///
	/// \param loop True to play in loop, false to play once
	///
	/// \see getLoop
	///
	////////////////////////////////////////////////////////////
	void setLoop( bool loop );

	////////////////////////////////////////////////////////////
	/// \brief Change the current playing position of the sound
	///
	/// The playing position can be changed when the sound is
	/// either paused or playing. Changing the playing position
	/// when the sound is stopped has no effect, since playing
	/// the sound will reset its position.
	///
	/// \param timeOffset New playing position, from the beginning of the sound
	///
	/// \see getPlayingOffset
	///
	////////////////////////////////////////////////////////////
	void setPlayingOffset( Time timeOffset );

	////////////////////////////////////////////////////////////
	/// \brief Get the audio buffer attached to the sound
	///
	/// \return Sound buffer attached to the sound (can be NULL)
	///
	////////////////////////////////////////////////////////////
	const SoundBuffer* getBuffer() const;

	////////////////////////////////////////////////////////////
	/// \brief Tell whether or not the sound is in loop mode
	///
	/// \return True if the sound is looping, false otherwise
	///
	/// \see setLoop
	///
	////////////////////////////////////////////////////////////
	bool getLoop() const;

	////////////////////////////////////////////////////////////
	/// \brief Get the current playing position of the sound
	///
	/// \return Current playing position, from the beginning of the sound
	///
	/// \see setPlayingOffset
	///
	////////////////////////////////////////////////////////////
	Time getPlayingOffset() const;

	////////////////////////////////////////////////////////////
	/// \brief Get the current status of the sound (stopped, paused, playing)
	///
	/// \return Current status of the sound
	///
	////////////////////////////////////////////////////////////
	Status getStatus() const;

	////////////////////////////////////////////////////////////
	/// \brief Overload of assignment operator
	///
	/// \param right Instance to assign
	///
	/// \return Reference to self
	///
	////////////////////////////////////////////////////////////
	Sound& operator=( const Sound& right );

	////////////////////////////////////////////////////////////
	/// \brief Reset the internal buffer of the sound
	///
	/// This function is for internal use only, you don't have
	/// to use it. It is called by the SoundBuffer that
	/// this sound uses, when it is destroyed in order to prevent
	/// the sound from using a dead buffer.
	///
	////////////////////////////////////////////////////////////
	void resetBuffer();

  private:
	const SoundBuffer* mBuffer; ///< Sound buffer bound to the source
};

}} // namespace EE::Audio

#endif

////////////////////////////////////////////////////////////
/// @class EE::Audio::Sound
///
/// Sound is the class to use to play sounds.
/// It provides:
/// \li Control (play, pause, stop)
/// \li Ability to modify output parameters in real-time (pitch, volume, ...)
/// \li 3D spatial features (position, attenuation, ...).
///
/// Sound is perfect for playing short sounds that can
/// fit in memory and require no latency, like foot steps or
/// gun shots. For longer sounds, like background musics
/// or long speeches, rather see Music (which is based
/// on streaming).
///
/// In order to work, a sound must be given a buffer of audio
/// data to play. Audio data (samples) is stored in SoundBuffer,
/// and attached to a sound with the setBuffer() function.
/// The buffer object attached to a sound must remain alive
/// as long as the sound uses it. Note that multiple sounds
/// can use the same sound buffer at the same time.
///
/// Usage example:
/// \code
/// SoundBuffer buffer;
/// buffer.loadFromFile("sound.wav");
///
/// Sound sound;
/// sound.setBuffer(buffer);
/// sound.play();
/// \endcode
///
/// \see SoundBuffer, Music
///
////////////////////////////////////////////////////////////
