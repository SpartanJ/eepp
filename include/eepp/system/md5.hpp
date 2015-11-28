#ifndef EE_SYSTEM_MD5_HPP
#define EE_SYSTEM_MD5_HPP

#include <eepp/system/iostream.hpp>

namespace EE { namespace System {

class MD5 {
	public:
		typedef struct {
			std::vector<Uint8> digest;

			std::string ToHexString() {
				return MD5::HexDigest( digest );
			}
		} Result;

		/** @return Calculates the md5 hash from a stream */
		static Result FromStream( IOStream& stream );

		/** Calculates the md5 hash from a file */
		static Result FromFile( std::string path );

		/** Calculates the md5 hash from memory */
		static Result FromMemory( const Uint8* data, Uint64 size );

		/** Calculates the md5 hash from a string */
		static Result FromString( const std::string& str );

		/** Calculates the md5 hash from a string */
		static Result FromString( const String& str );
	protected:
		typedef unsigned int MD5_u32plus;

		typedef struct {
			MD5_u32plus lo, hi;
			MD5_u32plus a, b, c, d;
			unsigned char buffer[64];
			MD5_u32plus block[16];
		} Context;

		static const void * Body( Context *ctx, const void *data, unsigned long size );

		static void Init( Context * ctx );

		static void Update( Context * ctx, const void *data, unsigned long size );

		static void Final( unsigned char *result, Context * ctx );

		static std::string HexDigest( std::vector<Uint8>& digest );
};

}}

#endif
