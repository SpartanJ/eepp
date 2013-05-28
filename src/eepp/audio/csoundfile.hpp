#ifndef EE_AUDIOCSOUNDFILE_H
#define EE_AUDIOCSOUNDFILE_H

#include <eepp/audio/base.hpp>

namespace EE { namespace Audio {

/** @brief Provide read and write access to sound files */
class EE_API cSoundFile {
	public:
		/** @brief Open a sound file for reading */
		static cSoundFile * CreateRead(const std::string& Filename);

		/** @brief Open a sound file from memory for reading */
		static cSoundFile * CreateRead(const char* Data, std::size_t SizeInBytes);

		/**	@brief Open a sound file for writing */
		static cSoundFile * CreateWrite(const std::string& Filename, unsigned int ChannelCount, unsigned int SampleRate);

		virtual ~cSoundFile();

		/** @brief Get the total number of audio samples in the file */
		std::size_t GetSamplesCount() const;

		/** @brief Get the number of channels used by the sound
		*	@return Number of channels (1 = mono, 2 = stereo)
		*/
		unsigned int GetChannelCount() const;

		/** @brief Get the sample rate of the sound
		*	@return Sample rate, in samples per second
		*/
		unsigned int GetSampleRate() const;

		/** @brief Restarts the audio from the beginning. */
		bool Restart();

		/** @brief Read audio samples from the loaded sound
		**	@param data        Pointer to the sample array to fill
		**	@param sampleCount Number of samples to read
		**	@return Number of samples actually read (may be less than \a sampleCount) */
		virtual std::size_t Read(Int16* Data, std::size_t NbSamples);

		/** @brief Write audio samples to the file
		**	@param data        Pointer to the sample array to write
		**	@param sampleCount Number of samples to write */
		virtual void Write(const Int16* Data, std::size_t NbSamples);

		/** @brief Change the current read position in the file
		**	@param timeOffset New playing position, from the beginning of the file */
		virtual void Seek( Uint32 timeOffset );
	protected :
		cSoundFile();

		virtual bool OpenRead(const std::string& Filename, std::size_t& NbSamples, unsigned int& ChannelCount, unsigned int& SampleRate);

		virtual bool OpenRead(const char* Data, std::size_t SizeInBytes, std::size_t& NbSamples, unsigned int& ChannelCount, unsigned int& SampleRate);

		virtual bool OpenWrite(const std::string& Filename, unsigned int ChannelCount, unsigned int SampleRate);

		std::size_t		mNbSamples;
		unsigned int	mChannelCount;
		unsigned int	mSampleRate;
		std::string		mFilename;
		const char *	mData;
		std::size_t		mSize;
};

}}

#endif
