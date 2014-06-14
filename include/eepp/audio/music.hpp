#ifndef EE_AUDIOCMUSIC_H
#define EE_AUDIOCMUSIC_H

#include <eepp/audio/base.hpp>
#include <eepp/audio/soundstream.hpp>

namespace EE { namespace Audio {

class SoundFile;

/** @brief Streamed music played from an audio file */
class EE_API Music : public SoundStream {
	public :
		/** Construct the music with a buffer size */
		Music( std::size_t BufferSize = 48000 );

		~Music();

		/** Open a Music file from a path */
		bool OpenFromFile( const std::string& Filename );

		/** Open a Music file from memory */
		bool OpenFromMemory( const char * Data, std::size_t SizeInBytes );

		/** Open a Music file from a file inside a pack file */
		bool OpenFromPack( cPack * Pack, const std::string& FilePackPath );

		/** Get the Music Duration */
		cTime GetDuration() const;
	private :
		virtual bool OnStart();

		virtual bool OnGetData(Chunk& Data);

		virtual void OnSeek( cTime timeOffset);

		SoundFile * 		mFile; 		///< Sound file
		float				mDuration; 	///< Music duration, in seconds
		std::vector<Int16>	mSamples; 	///< Temporary buffer of samples
		SafeDataPointer		mData;
};

}}

#endif
