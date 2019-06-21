#ifndef EE_SYSTEM_COMPRESSION_HPP
#define EE_SYSTEM_COMPRESSION_HPP

#include <eepp/config.hpp>
#include <eepp/system/iostream.hpp>

namespace EE { namespace System {

class EE_API Compression {
	public:
		enum Mode {
			MODE_DEFLATE,
			MODE_GZIP
		};

		enum Status {
			OK            = 0,
			ERRNO         = -1,
			STREAM_ERROR  = -2,
			DATA_ERROR    = -3,
			MEM_ERROR     = -4,
			BUF_ERROR     = -5,
			VERSION_ERROR = -6
		};

		struct ZlibConfig {
			int level= -1;
		};

		struct GzipConfig {
			int level = -1;
		};

		struct Config {
			Config() {}
			ZlibConfig zlib;
			GzipConfig gzip;
		};

		static Status compress(Uint8* dst, Uint64 dstMaxSize, const Uint8* src, Uint64 srcSize, Mode mode = MODE_DEFLATE, const Config& config = Config());

		static Status compress(IOStream& dst, IOStream& src, Mode mode = MODE_DEFLATE, const Config& config = Config());

		static int getMaxCompressedBufferSize(Uint64 srcSize, Mode mode = MODE_DEFLATE, const Config& config = Config());

		static Status decompress(Uint8* dst, Uint64 dstMaxSize, const Uint8* src, Uint64 srcSize, Mode mode = MODE_DEFLATE);

		static Status decompress(IOStream& dst, IOStream& src, Mode mode = MODE_DEFLATE);

		static std::size_t getModeDefaultChunkSize( const Mode& mode );
};

}}

#endif // EE_SYSTEM_COMPRESSION_HPP
