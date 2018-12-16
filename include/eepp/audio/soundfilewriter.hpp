#ifndef EE_AUDIO_SOUNDFILEWRITER_HPP
#define EE_AUDIO_SOUNDFILEWRITER_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace Audio {

/// \brief Abstract base class for sound file encoding
class EE_API SoundFileWriter {
	public:
		virtual ~SoundFileWriter() {}

		////////////////////////////////////////////////////////////
		/// \brief Open a sound file for writing
		///
		/// \param filename	 Path of the file to open
		/// \param sampleRate   Sample rate of the sound
		/// \param channelCount Number of channels of the sound
		///
		/// \return True if the file was successfully opened
		///
		////////////////////////////////////////////////////////////
		virtual bool open(const std::string& filename, unsigned int sampleRate, unsigned int channelCount) = 0;

		////////////////////////////////////////////////////////////
		/// \brief Write audio samples to the open file
		///
		/// \param samples Pointer to the sample array to write
		/// \param count   Number of samples to write
		///
		////////////////////////////////////////////////////////////
		virtual void write(const Int16* samples, Uint64 count) = 0;
};

}}

#endif

////////////////////////////////////////////////////////////
/// \class SoundFileWriter
/// \ingroup audio
///
/// This class allows users to write audio file formats not natively
/// supported by SFML, and thus extend the set of supported writable
/// audio formats.
///
/// A valid sound file writer must override the open and write functions,
/// as well as providing a static check function; the latter is used by
/// SFML to find a suitable writer for a given filename.
///
/// To register a new writer, use the SoundFileFactory::registerWriter
/// template function.
///
/// Usage example:
/// \code
/// class MySoundFileWriter : public SoundFileWriter
/// {
/// public:
///
///	 static bool check(const std::string& filename)
///	 {
///		 // typically, check the extension
///		 // return true if the writer can handle the format
///	 }
///
///	 virtual bool open(const std::string& filename, unsigned int sampleRate, unsigned int channelCount)
///	 {
///		 // open the file 'filename' for writing,
///		 // write the given sample rate and channel count to the file header
///		 // return true on success
///	 }
///
///	 virtual void write(const Int16* samples, Uint64 count)
///	 {
///		 // write 'count' samples stored at address 'samples',
///		 // convert them (for example to normalized float) if the format requires it
///	 }
/// };
///
/// SoundFileFactory::registerWriter<MySoundFileWriter>();
/// \endcode
///
/// \see OutputSoundFile, SoundFileFactory, SoundFileReader
///
////////////////////////////////////////////////////////////
