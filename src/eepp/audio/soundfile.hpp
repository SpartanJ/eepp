#ifndef EE_AUDIOCSOUNDFILE_H
#define EE_AUDIOCSOUNDFILE_H

#include <eepp/audio/base.hpp>

namespace EE { namespace Audio {

/** @brief Provide read and write access to sound files */
class EE_API SoundFile {
	public:
		/** @brief Open a sound file for reading */
		static SoundFile * createRead(const std::string& Filename);

		/** @brief Open a sound file from memory for reading */
		static SoundFile * createRead(const char* Data, std::size_t SizeInBytes);

		/**	@brief Open a sound file for writing */
		static SoundFile * createWrite(const std::string& Filename, unsigned int ChannelCount, unsigned int SampleRate);

		virtual ~SoundFile();

		/** @brief Get the total number of audio samples in the file */
		std::size_t getSamplesCount() const;

		/** @brief Get the number of channels used by the sound
		*	@return Number of channels (1 = mono, 2 = stereo)
		*/
		unsigned int getChannelCount() const;

		/** @brief Get the sample rate of the sound
		*	@return Sample rate, in samples per second
		*/
		unsigned int getSampleRate() const;

		/** @brief Restarts the audio from the beginning. */
		bool restart();

		/** @brief Read audio samples from the loaded sound
		**	@param data        Pointer to the sample array to fill
		**	@param sampleCount Number of samples to read
		**	@return Number of samples actually read (may be less than \a sampleCount) */
		virtual std::size_t read(Int16* Data, std::size_t SamplesCount);

		/** @brief Write audio samples to the file
		**	@param data        Pointer to the sample array to write
		**	@param sampleCount Number of samples to write */
		virtual void write(const Int16* Data, std::size_t SamplesCount);

		/** @brief Change the current read position in the file
		**	@param timeOffset New playing position, from the beginning of the file */
		virtual void seek( Time timeOffset );
	protected :
		SoundFile();

		virtual bool openRead(const std::string& Filename, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate);

		virtual bool openRead(const char* Data, std::size_t SizeInBytes, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate);

		virtual bool openWrite(const std::string& Filename, unsigned int ChannelCount, unsigned int SampleRate);

		std::size_t		mSamplesCount;
		unsigned int	mChannelCount;
		unsigned int	mSampleRate;
		std::string		mFilename;
		const char *	mData;
		std::size_t		mSize;
};

}}

#endif
