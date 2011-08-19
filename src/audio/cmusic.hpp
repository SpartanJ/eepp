#ifndef EE_AUDIOCMUSIC_H
#define EE_AUDIOCMUSIC_H

#include "base.hpp"
#include "csoundfile.hpp"
#include "csoundstream.hpp"
namespace EE { namespace Audio {

class EE_API cMusic : public cSoundStream {
	public :
		/** Construct the music with a buffer size */
		cMusic( std::size_t BufferSize = 44100 );

		~cMusic();

		/** Open a Music file from a path */
		bool OpenFromFile( const std::string& Filename );

		/** Open a Music file from memory */
		bool OpenFromMemory( const char * Data, std::size_t SizeInBytes );

		/** Open a Music file from a file inside a pack file */
		bool OpenFromPack( cPack * Pack, const std::string& FilePackPath );

		/** Get the Music Duration */
		eeFloat GetDuration() const;
	private :
		virtual bool OnStart();

		virtual bool OnGetData(Chunk& Data);

		virtual void OnSeek(float timeOffset);

		cSoundFile * 		mFile; 		///< Sound file
		eeFloat 			mDuration; 	///< Music duration, in seconds
		std::vector<Int16>	mSamples; 	///< Temporary buffer of samples
		SafeDataPointer		mData;
};

}}

#endif
