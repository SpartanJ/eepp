#ifndef EE_AUDIO_SOUNDBUFFER_HPP
#define EE_AUDIO_SOUNDBUFFER_HPP

#include <eepp/config.hpp>
#include <eepp/audio/alresource.hpp>
#include <eepp/system/time.hpp>
#include <string>
#include <vector>
#include <set>

namespace EE { namespace System {
class IOStream;
class Pack;
}}

using namespace EE::System;

namespace EE { namespace Audio {

class Sound;
class InputSoundFile;

/// \brief Storage for audio samples defining a sound
class EE_API SoundBuffer : AlResource {
	public:
		SoundBuffer();

		/// \brief Copy constructor
		/// \param copy Instance to copy
		SoundBuffer(const SoundBuffer& copy);

		~SoundBuffer();

		////////////////////////////////////////////////////////////
		/// \brief Load the sound buffer from a file
		///
		/// See the documentation of InputSoundFile for the list
		/// of supported formats.
		///
		/// \param filename Path of the sound file to load
		///
		/// \return True if loading succeeded, false if it failed
		///
		/// \see loadFromMemory, loadFromStream, loadFromSamples, saveToFile
		///
		////////////////////////////////////////////////////////////
		bool loadFromFile(const std::string& filename);

		////////////////////////////////////////////////////////////
		/// \brief Load the sound buffer from a file in memory
		///
		/// See the documentation of InputSoundFile for the list
		/// of supported formats.
		///
		/// \param data		Pointer to the file data in memory
		/// \param sizeInBytes Size of the data to load, in bytes
		///
		/// \return True if loading succeeded, false if it failed
		///
		/// \see loadFromFile, loadFromStream, loadFromSamples
		///
		////////////////////////////////////////////////////////////
		bool loadFromMemory(const void* data, std::size_t sizeInBytes);

		////////////////////////////////////////////////////////////
		/// \brief Load the sound buffer from a custom stream
		///
		/// See the documentation of InputSoundFile for the list
		/// of supported formats.
		///
		/// \param stream Source stream to read from
		///
		/// \return True if loading succeeded, false if it failed
		///
		/// \see loadFromFile, loadFromMemory, loadFromSamples
		///
		////////////////////////////////////////////////////////////
		bool loadFromStream(IOStream& stream);

		////////////////////////////////////////////////////////////
		/// \brief Load the sound buffer from an array of audio samples
		///
		/// The assumed format of the audio samples is 16 bits signed integer
		/// (Int16).
		///
		/// \param samples	  Pointer to the array of samples in memory
		/// \param sampleCount  Number of samples in the array
		/// \param channelCount Number of channels (1 = mono, 2 = stereo, ...)
		/// \param sampleRate   Sample rate (number of samples to play per second)
		///
		/// \return True if loading succeeded, false if it failed
		///
		/// \see loadFromFile, loadFromMemory, saveToFile
		///
		////////////////////////////////////////////////////////////
		bool loadFromSamples(const Int16* samples, Uint64 sampleCount, unsigned int channelCount, unsigned int sampleRate);

		bool loadFromPack( Pack * pack, std::string filePackPath );

		////////////////////////////////////////////////////////////
		/// \brief Save the sound buffer to an audio file
		///
		/// See the documentation of OutputSoundFile for the list
		/// of supported formats.
		///
		/// \param filename Path of the sound file to write
		///
		/// \return True if saving succeeded, false if it failed
		///
		/// \see loadFromFile, loadFromMemory, loadFromSamples
		///
		////////////////////////////////////////////////////////////
		bool saveToFile(const std::string& filename) const;

		////////////////////////////////////////////////////////////
		/// \brief Get the array of audio samples stored in the buffer
		///
		/// The format of the returned samples is 16 bits signed integer
		/// (Int16). The total number of samples in this array
		/// is given by the getSampleCount() function.
		///
		/// \return Read-only pointer to the array of sound samples
		///
		/// \see getSampleCount
		///
		////////////////////////////////////////////////////////////
		const Int16* getSamples() const;

		////////////////////////////////////////////////////////////
		/// \brief Get the number of samples stored in the buffer
		///
		/// The array of samples can be accessed with the getSamples()
		/// function.
		///
		/// \return Number of samples
		///
		/// \see getSamples
		///
		////////////////////////////////////////////////////////////
		Uint64 getSampleCount() const;

		////////////////////////////////////////////////////////////
		/// \brief Get the sample rate of the sound
		///
		/// The sample rate is the number of samples played per second.
		/// The higher, the better the quality (for example, 44100
		/// samples/s is CD quality).
		///
		/// \return Sample rate (number of samples per second)
		///
		/// \see getChannelCount, getDuration
		///
		////////////////////////////////////////////////////////////
		unsigned int getSampleRate() const;

		////////////////////////////////////////////////////////////
		/// \brief Get the number of channels used by the sound
		///
		/// If the sound is mono then the number of channels will
		/// be 1, 2 for stereo, etc.
		///
		/// \return Number of channels
		///
		/// \see getSampleRate, getDuration
		///
		////////////////////////////////////////////////////////////
		unsigned int getChannelCount() const;

		////////////////////////////////////////////////////////////
		/// \brief Get the total duration of the sound
		///
		/// \return Sound duration
		///
		/// \see getSampleRate, getChannelCount
		///
		////////////////////////////////////////////////////////////
		Time getDuration() const;

		////////////////////////////////////////////////////////////
		/// \brief Overload of assignment operator
		///
		/// \param right Instance to assign
		///
		/// \return Reference to self
		///
		////////////////////////////////////////////////////////////
		SoundBuffer& operator =(const SoundBuffer& right);

	private:

		friend class Sound;

		////////////////////////////////////////////////////////////
		/// \brief Initialize the internal state after loading a new sound
		///
		/// \param file Sound file providing access to the new loaded sound
		///
		/// \return True on successful initialization, false on failure
		///
		////////////////////////////////////////////////////////////
		bool initialize(InputSoundFile& file);

		////////////////////////////////////////////////////////////
		/// \brief Update the internal buffer with the cached audio samples
		///
		/// \param channelCount Number of channels
		/// \param sampleRate   Sample rate (number of samples per second)
		///
		/// \return True on success, false if any error happened
		///
		////////////////////////////////////////////////////////////
		bool update(unsigned int channelCount, unsigned int sampleRate);

		////////////////////////////////////////////////////////////
		/// \brief Add a sound to the list of sounds that use this buffer
		///
		/// \param sound Sound instance to attach
		///
		////////////////////////////////////////////////////////////
		void attachSound(Sound* sound) const;

		////////////////////////////////////////////////////////////
		/// \brief Remove a sound from the list of sounds that use this buffer
		///
		/// \param sound Sound instance to detach
		///
		////////////////////////////////////////////////////////////
		void detachSound(Sound* sound) const;

		////////////////////////////////////////////////////////////
		// Types
		////////////////////////////////////////////////////////////
		typedef std::set<Sound*> SoundList; ///< Set of unique sound instances

		////////////////////////////////////////////////////////////
		// Member data
		////////////////////////////////////////////////////////////
		unsigned int	   mBuffer;   ///< OpenAL buffer identifier
		std::vector<Int16> mSamples;  ///< Samples buffer
		Time			   mDuration; ///< Sound duration
		mutable SoundList  mSounds;   ///< List of sounds that are using this buffer
};

}}

#endif


////////////////////////////////////////////////////////////
/// @class EE::Audio::SoundBuffer
///
/// A sound buffer holds the data of a sound, which is
/// an array of audio samples. A sample is a 16 bits signed integer
/// that defines the amplitude of the sound at a given time.
/// The sound is then reconstituted by playing these samples at
/// a high rate (for example, 44100 samples per second is the
/// standard rate used for playing CDs). In short, audio samples
/// are like texture pixels, and a SoundBuffer is similar to
/// a Texture.
///
/// A sound buffer can be loaded from a file (see loadFromFile()
/// for the complete list of supported formats), from memory, from
/// a custom stream (see IOStream) or directly from an array
/// of samples. It can also be saved back to a file.
///
/// Sound buffers alone are not very useful: they hold the audio data
/// but cannot be played. To do so, you need to use the Sound class,
/// which provides functions to play/pause/stop the sound as well as
/// changing the way it is outputted (volume, pitch, 3D position, ...).
/// This separation allows more flexibility and better performances:
/// indeed a SoundBuffer is a heavy resource, and any operation on it
/// is slow (often too slow for real-time applications). On the other
/// side, a Sound is a lightweight object, which can use the audio data
/// of a sound buffer and change the way it is played without actually
/// modifying that data. Note that it is also possible to bind
/// several Sound instances to the same SoundBuffer.
///
/// It is important to note that the Sound instance doesn't
/// copy the buffer that it uses, it only keeps a reference to it.
/// Thus, a SoundBuffer must not be destructed while it is
/// used by a Sound (i.e. never write a function that
/// uses a local SoundBuffer instance for loading a sound).
///
/// Usage example:
/// \code
/// // Declare a new sound buffer
/// SoundBuffer buffer;
///
/// // Load it from a file
/// if (!buffer.loadFromFile("sound.wav"))
/// {
///	 // error...
/// }
///
/// // Create a sound source and bind it to the buffer
/// Sound sound1;
/// sound1.setBuffer(buffer);
///
/// // Play the sound
/// sound1.play();
///
/// // Create another sound source bound to the same buffer
/// Sound sound2;
/// sound2.setBuffer(buffer);
///
/// // Play it with a higher pitch -- the first sound remains unchanged
/// sound2.setPitch(2);
/// sound2.play();
/// \endcode
///
/// \see Sound, SoundBufferRecorder
///
////////////////////////////////////////////////////////////
