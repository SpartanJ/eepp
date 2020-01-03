#ifndef EE_AUDIO_OUTPUTSOUNDFILE_HPP
#define EE_AUDIO_OUTPUTSOUNDFILE_HPP

#include <eepp/config.hpp>
#include <eepp/core/noncopyable.hpp>
#include <string>

namespace EE { namespace Audio {

class SoundFileWriter;

/// \brief Provide write access to sound files
class EE_API OutputSoundFile : NonCopyable {
	public:
		OutputSoundFile();

		~OutputSoundFile();

		////////////////////////////////////////////////////////////
		/// \brief Open the sound file from the disk for writing
		///
		/// The supported audio formats are: WAV, OGG/Vorbis, FLAC.
		///
		/// \param filename	 Path of the sound file to write
		/// \param sampleRate   Sample rate of the sound
		/// \param channelCount Number of channels in the sound
		///
		/// \return True if the file was successfully opened
		///
		////////////////////////////////////////////////////////////
		bool openFromFile(const std::string& filename, unsigned int sampleRate, unsigned int channelCount);

		////////////////////////////////////////////////////////////
		/// \brief Write audio samples to the file
		///
		/// \param samples	 Pointer to the sample array to write
		/// \param count	   Number of samples to write
		///
		////////////////////////////////////////////////////////////
		void write(const Int16* samples, Uint64 count);

	private:
		void close();

		SoundFileWriter* mWriter; ///< Writer that handles I/O on the file's format
};

}}

#endif

////////////////////////////////////////////////////////////
/// @class EE::Audio::OutputSoundFile
///
/// This class encodes audio samples to a sound file. It is
/// used internally by higher-level classes such as SoundBuffer,
/// but can also be useful if you want to create audio files from
/// custom data sources, like generated audio samples.
///
/// Usage example:
/// \code
/// // Create a sound file, ogg/vorbis format, 44100 Hz, stereo
/// OutputSoundFile file;
/// if (!file.openFromFile("music.ogg", 44100, 2))
///	 /* error */;
///
/// while (...)
/// {
///	 // Read or generate audio samples from your custom source
///	 std::vector<Int16> samples = ...;
///
///	 // Write them to the file
///	 file.write(samples.data(), samples.size());
/// }
/// \endcode
///
/// \see SoundFileWriter, InputSoundFile
///
////////////////////////////////////////////////////////////
