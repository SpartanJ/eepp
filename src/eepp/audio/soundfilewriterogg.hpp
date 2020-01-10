#ifndef EE_AUDIO_SOUNDFILEWRITEROGG_HPP
#define EE_AUDIO_SOUNDFILEWRITEROGG_HPP

#include <eepp/audio/soundfilewriter.hpp>
#include <fstream>
#include <vorbis/vorbisenc.h>

namespace EE { namespace Audio { namespace Private {

/// \brief Implementation of sound file writer that handles OGG/Vorbis files
class SoundFileWriterOgg : public SoundFileWriter {
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
	SoundFileWriterOgg();

	~SoundFileWriterOgg();

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
	/// \brief Flush blocks produced by the ogg stream, if any
	void flushBlocks();

	void close();

	unsigned int mChannelCount; // channel count of the sound being written
	std::ofstream mFile;		// output file
	ogg_stream_state mOgg;		// ogg stream
	vorbis_info mVorbis;		// vorbis handle
	vorbis_dsp_state mState;	// current encoding state
};

}}} // namespace EE::Audio::Private

#endif
