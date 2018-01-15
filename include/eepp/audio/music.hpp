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
		bool openFromFile( const std::string& Filename );

		/** Open a Music file from memory */
		bool openFromMemory( const char * Data, std::size_t SizeInBytes );

		/** Open a Music file from a file inside a pack file */
		bool openFromPack( Pack * Pack, const std::string& FilePackPath );

		/** Get the Music Duration */
		Time getDuration() const;
	private :
		virtual bool onStart();

		virtual bool onGetData(Chunk& Data);

		virtual void onSeek( Time timeOffset);

		SoundFile * 		mFile; 		///< Sound file
		float				mDuration; 	///< Music duration, in seconds
		std::vector<Int16>	mSamples; 	///< Temporary buffer of samples
		SafeDataPointer		mData;
};

}}

#endif
