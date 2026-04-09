#include <eepp/core/debug.hpp>
#include <eepp/system/compression.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/scopedbuffer.hpp>

#include <brotli/decode.h>
// eepp only brings decoder implementation, encoding won't be available for the moment
// #include <brotli/encode.h>
#include <zlib.h>

#define DEFLATE_CHUNK_SIZE ( 16384 )

namespace EE { namespace System {

Compression::Status Compression::compress( Uint8* dst, Uint64 dstMaxSize, const Uint8* src,
										   Uint64 srcSize, Mode mode, const Config& config ) {
	IOStreamMemory srcMem( (const char*)src, srcSize );
	IOStreamMemory dstMem( (char*)dst, dstMaxSize );
	return compress( dstMem, srcMem, mode, config );
}

Compression::Status Compression::compress( IOStream& dst, IOStream& src, Compression::Mode mode,
										   const Config& config ) {
	switch ( mode ) {
		case MODE_BROTLI: {
/*
			BrotliEncoderState* state = BrotliEncoderCreateInstance( nullptr, nullptr, nullptr );
			if ( !state )
				return Status::MEM_ERROR;

			int quality =
				config.brotli.quality == -1 ? BROTLI_DEFAULT_QUALITY : config.brotli.quality;
			int windowBits =
				config.brotli.windowBits == -1 ? BROTLI_DEFAULT_WINDOW : config.brotli.windowBits;

			BrotliEncoderSetParameter( state, BROTLI_PARAM_QUALITY, quality );
			BrotliEncoderSetParameter( state, BROTLI_PARAM_LGWIN, windowBits );

			src.seek( 0 );

			char in[DEFLATE_CHUNK_SIZE];
			char out[DEFLATE_CHUNK_SIZE];

			bool isEof = false;

			while ( !isEof ) {
				size_t bytesRead = src.read( in, DEFLATE_CHUNK_SIZE );
				isEof = src.tell() == src.getSize();

				const uint8_t* next_in = reinterpret_cast<const uint8_t*>( in );
				size_t avail_in = bytesRead;

				while ( avail_in > 0 || ( isEof && !BrotliEncoderIsFinished( state ) ) ) {
					uint8_t* next_out = reinterpret_cast<uint8_t*>( out );
					size_t avail_out = DEFLATE_CHUNK_SIZE;

					BrotliEncoderOperation op =
						isEof ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS;

					if ( !BrotliEncoderCompressStream( state, op, &avail_in, &next_in, &avail_out,
													   &next_out, nullptr ) ) {
						BrotliEncoderDestroyInstance( state );
						return Status::STREAM_ERROR;
					}

					size_t have = DEFLATE_CHUNK_SIZE - avail_out;
					if ( have > 0 ) {
						if ( dst.write( out, have ) != (ios_size)have ) {
							BrotliEncoderDestroyInstance( state );
							return Status::ERRNO;
						}
					}
				}
			}

			BrotliEncoderDestroyInstance( state );
			return Status::OK;
*/
			return Status::VERSION_ERROR;
		}
		case MODE_DEFLATE:
		case MODE_GZIP: {
			int ret, flush;
			ios_size have;
			z_stream strm = {};
			char in[DEFLATE_CHUNK_SIZE];
			char out[DEFLATE_CHUNK_SIZE];
			int level = mode == MODE_DEFLATE ? config.zlib.level : config.gzip.level;
			int windowBits = mode == Compression::MODE_DEFLATE ? MAX_WBITS : MAX_WBITS | 16;

			ret = deflateInit2( &strm, level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY );
			if ( ret != Z_OK )
				return (Status)ret;

			src.seek( 0 );

			do {
				strm.avail_in = src.read( in, DEFLATE_CHUNK_SIZE );
				if ( strm.avail_in == 0 ) {
					deflateEnd( &strm );
					return Status::ERRNO;
				}

				flush = src.tell() == src.getSize() ? Z_FINISH : Z_NO_FLUSH;
				strm.next_in = (unsigned char*)in;

				do {
					strm.avail_out = DEFLATE_CHUNK_SIZE;
					strm.next_out = (unsigned char*)out;

					ret = deflate( &strm, flush );

					if ( ret == Z_STREAM_ERROR )
						return Status::STREAM_ERROR;

					have = DEFLATE_CHUNK_SIZE - strm.avail_out;

					if ( dst.write( out, have ) != have ) {
						deflateEnd( &strm );

						return Status::ERRNO;
					}
				} while ( strm.avail_out == 0 );

				if ( strm.avail_in != 0 )
					return Status::DATA_ERROR;
			} while ( flush != Z_FINISH );
		}
	}

	return Status::OK;
}

int Compression::getMaxCompressedBufferSize( Uint64 srcSize, Mode mode, const Config& ) {
	switch ( mode ) {
		case MODE_BROTLI: {
			// return BrotliEncoderMaxCompressedSize( srcSize );
			break;
		}
		case MODE_DEFLATE:
		case MODE_GZIP: {
			int windowBits = mode == MODE_DEFLATE ? MAX_WBITS : MAX_WBITS | 16;

			z_stream strm = {};
			int err = deflateInit2( &strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits, 8,
									Z_DEFAULT_STRATEGY );
			if ( err != Z_OK )
				return -1;
			int out = deflateBound( &strm, srcSize );
			deflateEnd( &strm );
			return out;
		}
	}

	return -1;
}

Compression::Status Compression::decompress( Uint8* dst, Uint64 dstMaxSize, const Uint8* src,
											 Uint64 srcSize, Mode mode ) {
	IOStreamMemory srcMem( (const char*)src, srcSize );
	IOStreamMemory dstMem( (char*)dst, dstMaxSize );
	return decompress( dstMem, srcMem, mode );
}

Compression::Status Compression::decompress( IOStream& dst, IOStream& src, Mode mode ) {
	switch ( mode ) {
		case MODE_BROTLI: {
			BrotliDecoderState* state = BrotliDecoderCreateInstance( nullptr, nullptr, nullptr );
			if ( !state )
				return Status::MEM_ERROR;

			ScopedBuffer buffer( DEFLATE_CHUNK_SIZE );
			ScopedBuffer bufferDst( DEFLATE_CHUNK_SIZE );

			src.seek( 0 );

			BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
			size_t totalSize = src.getSize();
			size_t totalRead = 0;

			while ( totalRead < totalSize || result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT ) {
				size_t bytesRead = 0;
				if ( result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT ) {
					bytesRead = src.read( (char*)buffer.get(), buffer.length() );
					totalRead += bytesRead;
				}

				const uint8_t* next_in = reinterpret_cast<const uint8_t*>( buffer.get() );
				size_t avail_in = bytesRead;

				while ( avail_in > 0 || result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT ) {
					uint8_t* next_out = reinterpret_cast<uint8_t*>( bufferDst.get() );
					size_t avail_out = bufferDst.length();

					result = BrotliDecoderDecompressStream( state, &avail_in, &next_in, &avail_out,
															&next_out, nullptr );

					if ( result == BROTLI_DECODER_RESULT_ERROR ) {
						BrotliDecoderDestroyInstance( state );
						return Status::DATA_ERROR;
					}

					size_t have = bufferDst.length() - avail_out;
					if ( have > 0 ) {
						dst.write( (const char*)bufferDst.get(), have );
					}

					if ( result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT ||
						 result == BROTLI_DECODER_RESULT_SUCCESS ) {
						break;
					}
				}

				if ( result == BROTLI_DECODER_RESULT_SUCCESS ) {
					break;
				}
			}

			BrotliDecoderDestroyInstance( state );
			return Status::OK;
		}
		case MODE_DEFLATE:
		case MODE_GZIP: {
			ScopedBuffer buffer( DEFLATE_CHUNK_SIZE );
			ScopedBuffer bufferDst( DEFLATE_CHUNK_SIZE );

			src.seek( 0 );

			int windowBits = mode == Compression::MODE_DEFLATE ? MAX_WBITS : MAX_WBITS | 16;

			z_stream strm = {};
			strm.next_in = buffer.get();

			int err = inflateInit2( &strm, windowBits );
			if ( err != Z_OK )
				return (Status)err;

			int zlibStatus;
			int bytesRead;
			unsigned int have;
			Uint32 totalSize = src.getSize();
			Uint32 totalRead = 0;

			while ( totalRead < totalSize ) {
				bytesRead = src.read( (char*)buffer.get(), buffer.length() );

				strm.avail_in = bytesRead;
				strm.next_in = buffer.get();

				do {
					strm.avail_out = bufferDst.length();
					strm.next_out = bufferDst.get();
					zlibStatus = inflate( &strm, Z_NO_FLUSH );

					switch ( zlibStatus ) {
						case Z_OK:
						case Z_STREAM_END:
						case Z_BUF_ERROR:
							break;
						default:
							inflateEnd( &strm );
							return (Status)zlibStatus;
					}

					have = bufferDst.length() - strm.avail_out;

					dst.write( (const char*)bufferDst.get(), have );
				} while ( strm.avail_out == 0 );

				totalRead += bytesRead;
			}

			inflateEnd( &strm );

			return Status::OK;
		}
	}

	return Status::ERRNO;
}

std::size_t Compression::getModeDefaultChunkSize( const Mode& ) {
	return DEFLATE_CHUNK_SIZE;
}

}} // namespace EE::System
