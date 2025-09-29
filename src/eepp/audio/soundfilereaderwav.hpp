#ifndef EE_AUDIO_SOUNDFILEREADERWAV_HPP
#define EE_AUDIO_SOUNDFILEREADERWAV_HPP

#include <eepp/audio/soundfilereader.hpp>
#include <string>

namespace EE { namespace Audio { namespace Private {

/// \brief Implementation of sound file reader that handles wav files
class SoundFileReaderWav : public SoundFileReader {
  public:
	////////////////////////////////////////////////////////////
	/// \brief Check if this reader can handle a file given by an input stream
	///
	/// \param stream Source stream to check
	///
	/// \return True if the file is supported by this reader
	///
	////////////////////////////////////////////////////////////
	static bool check( IOStream& stream );

	static bool usesFileExtension( std::string_view ext );

  public:
	////////////////////////////////////////////////////////////
	/// \brief Default constructor
	///
	////////////////////////////////////////////////////////////
	SoundFileReaderWav();

	////////////////////////////////////////////////////////////
	/// \brief Open a sound file for reading
	///
	/// \param stream Stream to open
	/// \param info   Structure to fill with the attributes of the loaded sound
	///
	////////////////////////////////////////////////////////////
	virtual bool open( IOStream& stream, Info& info );

	////////////////////////////////////////////////////////////
	/// \brief Change the current read position to the given sample offset
	///
	/// The sample offset takes the channels into account.
	/// If you have a time offset instead, you can easily find
	/// the corresponding sample offset with the following formula:
	/// `timeInSeconds * sampleRate * channelCount`
	/// If the given offset exceeds to total number of samples,
	/// this function must jump to the end of the file.
	///
	/// \param sampleOffset Index of the sample to jump to, relative to the beginning
	///
	////////////////////////////////////////////////////////////
	virtual void seek( Uint64 sampleOffset );

	////////////////////////////////////////////////////////////
	/// \brief Read audio samples from the open file
	///
	/// \param samples  Pointer to the sample array to fill
	/// \param maxCount Maximum number of samples to read
	///
	/// \return Number of samples actually read (may be less than \a maxCount)
	///
	////////////////////////////////////////////////////////////
	virtual Uint64 read( Int16* samples, Uint64 maxCount );

  private:
	////////////////////////////////////////////////////////////
	/// \brief Read the header of the open file
	///
	/// \param info Attributes of the sound file
	///
	/// \return True on success, false on error
	///
	////////////////////////////////////////////////////////////
	bool parseHeader( Info& info );

	////////////////////////////////////////////////////////////
	// Member data
	////////////////////////////////////////////////////////////
	IOStream* mStream;			  ///< Source stream to read from
	unsigned int mBytesPerSample; ///< Size of a sample, in bytes
	Uint64 mDataStart;			  ///< Starting position of the audio data in the open file
	Uint64 mDataEnd; ///< Position one byte past the end of the audio data in the open file
};

}}} // namespace EE::Audio::Private

#endif
