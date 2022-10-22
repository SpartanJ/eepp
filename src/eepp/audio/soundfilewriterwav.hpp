#ifndef EE_AUDIO_SOUNDFILEWRITERWAV_HPP
#define EE_AUDIO_SOUNDFILEWRITERWAV_HPP

#include <eepp/audio/soundfilewriter.hpp>
#include <fstream>
#include <string>

namespace EE { namespace Audio { namespace Private {

/// \brief Implementation of sound file writer that handles wav files
class SoundFileWriterWav : public SoundFileWriter {
  public:
	////////////////////////////////////////////////////////////
	/// \brief Check if this writer can handle a file on disk
	///
	/// \param filename Path of the sound file to check
	///
	/// \return True if the file can be written by this writer
	///
	////////////////////////////////////////////////////////////
	static bool check( const std::string& filename );

  public:
	SoundFileWriterWav();

	~SoundFileWriterWav();

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
	virtual bool open( const std::string& filename, unsigned int sampleRate,
					   unsigned int channelCount );

	////////////////////////////////////////////////////////////
	/// \brief Write audio samples to the open file
	///
	/// \param samples Pointer to the sample array to write
	/// \param count   Number of samples to write
	///
	////////////////////////////////////////////////////////////
	virtual void write( const Int16* samples, Uint64 count );

  private:
	////////////////////////////////////////////////////////////
	/// \brief Write the header of the open file
	///
	/// \param sampleRate   Sample rate of the sound
	/// \param channelCount Number of channels of the sound
	///
	/// \return True on success, false on error
	///
	////////////////////////////////////////////////////////////
	bool writeHeader( unsigned int sampleRate, unsigned int channelCount );

	////////////////////////////////////////////////////////////
	/// \brief Close the file
	///
	////////////////////////////////////////////////////////////
	void close();

	std::ofstream mFile; ///< File stream to write to
};

}}} // namespace EE::Audio::Private

#endif
