#include <eepp/system/iostreamdeflate.hpp>

#include <zlib.h>

namespace EE { namespace System {

struct LocalStreamData {
	z_stream strm;
	int state;
	bool writtenStream;
};

IOStreamDeflate* IOStreamDeflate::New( IOStream& inOutStream, Compression::Mode mode,
									   const Compression::Config& config ) {
	return eeNew( IOStreamDeflate, ( inOutStream, mode ) );
}

IOStreamDeflate::IOStreamDeflate( IOStream& inOutStream, Compression::Mode mode,
								  const Compression::Config& config ) :
	mStream( inOutStream ),
	mMode( mode ),
	mBuffer( Compression::getModeDefaultChunkSize( mode ) ),
	mLocalStream( eeNew( LocalStreamData, () ) ) {
	int windowBits = mode == Compression::MODE_DEFLATE ? MAX_WBITS : MAX_WBITS | 16;
	int level = mode == Compression::MODE_DEFLATE ? config.zlib.level : config.gzip.level;

	mLocalStream->strm = z_stream{};
	mLocalStream->writtenStream = false;

	mLocalStream->state =
		deflateInit2( &mLocalStream->strm, level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY );
}

IOStreamDeflate::~IOStreamDeflate() {
	if ( mStream.isOpen() && mLocalStream->writtenStream ) {
		z_stream& zstr = mLocalStream->strm;

		if ( zstr.next_out ) {
			int rc = deflate( &zstr, Z_FINISH );

			if ( rc != Z_OK && rc != Z_STREAM_END )
				return;

			mStream.write( (char*)mBuffer.get(), mBuffer.length() - zstr.avail_out );

			if ( !mStream.isOpen() )
				return;

			zstr.next_out = (unsigned char*)mBuffer.get();
			zstr.avail_out = mBuffer.length();

			while ( rc != Z_STREAM_END ) {
				rc = deflate( &zstr, Z_FINISH );

				if ( rc != Z_OK && rc != Z_STREAM_END )
					return;

				mStream.write( (char*)mBuffer.get(), mBuffer.length() - zstr.avail_out );

				if ( !mStream.isOpen() )
					return;

				zstr.next_out = (unsigned char*)mBuffer.get();
				zstr.avail_out = mBuffer.length();
			}
		}
	}

	deflateEnd( &mLocalStream->strm );

	eeSAFE_DELETE( mLocalStream );
}

ios_size IOStreamDeflate::read( char* buffer, ios_size length ) {
	if ( mLocalStream->state != Z_OK || !mStream.isOpen() )
		return 0;

	z_stream& zstr = mLocalStream->strm;

	bool eof = false;

	if ( zstr.avail_in == 0 ) {
		ios_size n = 0;

		if ( mStream.isOpen() ) {
			n = mStream.read( (char*)mBuffer.get(), mBuffer.length() );
		}

		if ( n > 0 ) {
			zstr.next_in = (unsigned char*)mBuffer.get();
			zstr.avail_in = n;
		} else {
			zstr.next_in = NULL;
			zstr.avail_in = 0;
			eof = true;
		}
	}

	zstr.next_out = (unsigned char*)buffer;
	zstr.avail_out = length;

	for ( ;; ) {
		int rc = deflate( &zstr, eof ? Z_FINISH : Z_NO_FLUSH );

		if ( eof && rc == Z_STREAM_END ) {
			return length - zstr.avail_out;
		}

		if ( rc != Z_OK )
			return 0;

		if ( zstr.avail_out == 0 )
			return static_cast<int>( length );

		if ( zstr.avail_in == 0 ) {
			ios_size n = 0;

			if ( mStream.isOpen() ) {
				n = mStream.read( (char*)mBuffer.get(), mBuffer.length() );
			}

			if ( n > 0 ) {
				zstr.next_in = (unsigned char*)mBuffer.get();
				zstr.avail_in = n;
			} else {
				zstr.next_in = NULL;
				zstr.avail_in = 0;
				eof = true;
			}
		}
	}
}

ios_size IOStreamDeflate::write( const char* buffer, ios_size length ) {
	mLocalStream->writtenStream = true;

	if ( mLocalStream->state != Z_OK || !mStream.isOpen() || length == 0 )
		return 0;

	z_stream& zstr = mLocalStream->strm;

	zstr.next_in = (unsigned char*)buffer;
	zstr.avail_in = length;
	zstr.next_out = mBuffer.get();
	zstr.avail_out = mBuffer.length();

	for ( ;; ) {
		int rc = deflate( &zstr, Z_NO_FLUSH );

		if ( rc != Z_OK )
			return 0;

		if ( zstr.avail_out == 0 ) {
			ios_size ret = mStream.write( (const char*)mBuffer.get(), mBuffer.length() );

			if ( ret == 0 )
				return 0;

			zstr.next_out = (unsigned char*)mBuffer.get();
			zstr.avail_out = mBuffer.length();
		}

		if ( zstr.avail_in == 0 ) {
			ios_size ret =
				mStream.write( (const char*)mBuffer.get(), mBuffer.length() - zstr.avail_out );

			if ( ret == 0 )
				return 0;

			zstr.next_out = (unsigned char*)mBuffer.get();
			zstr.avail_out = mBuffer.length();

			break;
		}
	}

	return length;
}

ios_size IOStreamDeflate::seek( ios_size position ) {
	return mStream.seek( position );
}

ios_size IOStreamDeflate::tell() {
	return mStream.tell();
}

ios_size IOStreamDeflate::getSize() {
	return mStream.getSize();
}

bool IOStreamDeflate::isOpen() {
	return mStream.isOpen();
}

const Compression::Mode& IOStreamDeflate::getMode() const {
	return mMode;
}

}} // namespace EE::System
