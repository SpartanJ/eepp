#include <eepp/system/iostreaminflate.hpp>

#include <brotli/decode.h>
#include <zlib.h>

namespace EE { namespace System {

struct LocalInflateStreamData {
	z_stream strm;
	int state;
	BrotliDecoderState* brotliState;
	BrotliDecoderResult brotliResult;
	size_t brotliAvailIn;
	const uint8_t* brotliNextIn;
};

IOStreamInflate* IOStreamInflate::New( IOStream& inOutStream, Compression::Mode mode ) {
	return eeNew( IOStreamInflate, ( inOutStream, mode ) );
}

IOStreamInflate::IOStreamInflate( IOStream& inOutStream, Compression::Mode mode ) :
	mStream( inOutStream ),
	mMode( mode ),
	mBuffer( Compression::getModeDefaultChunkSize( mode ) ),
	mLocalStream( eeNew( LocalInflateStreamData, () ) ) {

	if ( mode == Compression::MODE_BROTLI ) {
		mLocalStream->brotliState = BrotliDecoderCreateInstance( nullptr, nullptr, nullptr );
		mLocalStream->brotliResult = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
		mLocalStream->brotliAvailIn = 0;
		mLocalStream->brotliNextIn = nullptr;
		mLocalStream->state = mLocalStream->brotliState ? Z_OK : Z_MEM_ERROR;
	} else {
		int windowBits = mode == Compression::MODE_DEFLATE ? MAX_WBITS : MAX_WBITS | 16;
		mLocalStream->strm = z_stream{};
		mLocalStream->state = inflateInit2( &mLocalStream->strm, windowBits );
	}
}

IOStreamInflate::~IOStreamInflate() {
	if ( mMode == Compression::MODE_BROTLI ) {
		if ( mLocalStream->brotliState )
			BrotliDecoderDestroyInstance( mLocalStream->brotliState );
	} else {
		inflateEnd( &mLocalStream->strm );
	}

	eeSAFE_DELETE( mLocalStream );
}

ios_size IOStreamInflate::read( char* buffer, ios_size length ) {
	if ( mLocalStream->state != Z_OK || !mStream.isOpen() || length == 0 )
		return 0;

	if ( mMode == Compression::MODE_BROTLI ) {
		size_t avail_out = length;
		uint8_t* next_out = reinterpret_cast<uint8_t*>( buffer );

		while ( avail_out > 0 ) {
			if ( mLocalStream->brotliAvailIn == 0 &&
				 mLocalStream->brotliResult != BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT ) {
				ios_size n = 0;
				if ( mStream.isOpen() ) {
					n = mStream.read( (char*)mBuffer.get(), mBuffer.length() );
				}
				if ( n == 0 )
					break;
				mLocalStream->brotliAvailIn = n;
				mLocalStream->brotliNextIn = reinterpret_cast<const uint8_t*>( mBuffer.get() );
			}

			mLocalStream->brotliResult = BrotliDecoderDecompressStream(
				mLocalStream->brotliState, &mLocalStream->brotliAvailIn,
				&mLocalStream->brotliNextIn, &avail_out, &next_out, nullptr );

			if ( mLocalStream->brotliResult == BROTLI_DECODER_RESULT_ERROR ) {
				mLocalStream->state = Z_DATA_ERROR;
				return 0;
			}

			if ( mLocalStream->brotliResult == BROTLI_DECODER_RESULT_SUCCESS ) {
				mLocalStream->state = Z_STREAM_END;
				break;
			}
		}

		return length - avail_out;
	}

	z_stream& zstr = mLocalStream->strm;

	if ( zstr.avail_in == 0 ) {
		ios_size n = 0;

		if ( mStream.isOpen() ) {
			n = mStream.read( (char*)mBuffer.get(), mBuffer.length() );
		}

		zstr.next_in = (unsigned char*)mBuffer.get();
		zstr.avail_in = n;
	}

	zstr.next_out = (unsigned char*)buffer;
	zstr.avail_out = length;

	for ( ;; ) {
		int rc = inflate( &zstr, Z_NO_FLUSH );

		if ( rc == Z_DATA_ERROR ) {
			if ( zstr.avail_in == 0 ) {
				if ( mStream.isOpen() )
					rc = Z_OK;
				else
					rc = Z_STREAM_END;
			}
		}

		if ( rc == Z_STREAM_END ) {
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
				return length - zstr.avail_out;
			}
		}
	}
}

ios_size IOStreamInflate::write( const char* buffer, ios_size length ) {
	if ( length == 0 )
		return 0;
	if ( mLocalStream->state != Z_OK || !mStream.isOpen() || length < 0 )
		return -1;
	mHasInput = true;

	if ( mMode == Compression::MODE_BROTLI ) {
		size_t availIn = length;
		const uint8_t* nextIn = reinterpret_cast<const uint8_t*>( buffer );
		do {
			size_t availOut = mBuffer.length();
			uint8_t* nextOut = reinterpret_cast<uint8_t*>( mBuffer.get() );
			mLocalStream->brotliResult = BrotliDecoderDecompressStream(
				mLocalStream->brotliState, &availIn, &nextIn, &availOut, &nextOut, nullptr );
			const ios_size have = mBuffer.length() - availOut;
			if ( mLocalStream->brotliResult == BROTLI_DECODER_RESULT_ERROR ||
				 ( have > 0 && mStream.write( reinterpret_cast<const char*>( mBuffer.get() ),
											  have ) != have ) ) {
				mLocalStream->state = Z_DATA_ERROR;
				return -1;
			}
			if ( mLocalStream->brotliResult == BROTLI_DECODER_RESULT_SUCCESS ) {
				mLocalStream->state = availIn == 0 ? Z_STREAM_END : Z_DATA_ERROR;
				return availIn == 0 ? length : -1;
			}
		} while ( availIn > 0 ||
				  mLocalStream->brotliResult == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT );
		return length;
	}

	z_stream& stream = mLocalStream->strm;
	stream.next_in = reinterpret_cast<Bytef*>( const_cast<char*>( buffer ) );
	stream.avail_in = length;
	do {
		stream.next_out = mBuffer.get();
		stream.avail_out = mBuffer.length();
		const int result = inflate( &stream, Z_NO_FLUSH );
		const ios_size have = mBuffer.length() - stream.avail_out;
		if ( ( result != Z_OK && result != Z_STREAM_END && result != Z_BUF_ERROR ) ||
			 ( have > 0 &&
			   mStream.write( reinterpret_cast<const char*>( mBuffer.get() ), have ) != have ) ) {
			mLocalStream->state = Z_DATA_ERROR;
			return -1;
		}
		if ( result == Z_STREAM_END ) {
			mLocalStream->state = stream.avail_in == 0 ? Z_STREAM_END : Z_DATA_ERROR;
			return stream.avail_in == 0 ? length : -1;
		}
		if ( result == Z_BUF_ERROR && have == 0 )
			break;
	} while ( stream.avail_in > 0 || stream.avail_out == 0 );
	return length;
}

ios_size IOStreamInflate::seek( ios_size position ) {
	return mStream.seek( position );
}

ios_size IOStreamInflate::tell() {
	return mStream.tell();
}

ios_size IOStreamInflate::getSize() {
	return mStream.getSize();
}

bool IOStreamInflate::isOpen() {
	return mStream.isOpen() && mLocalStream->state != Z_STREAM_END;
}

bool IOStreamInflate::isComplete() const {
	return mLocalStream->state == Z_STREAM_END;
}

bool IOStreamInflate::hasError() const {
	return mLocalStream->state != Z_OK && mLocalStream->state != Z_STREAM_END;
}

bool IOStreamInflate::hasInput() const {
	return mHasInput;
}

const Compression::Mode& IOStreamInflate::getMode() const {
	return mMode;
}

}} // namespace EE::System
