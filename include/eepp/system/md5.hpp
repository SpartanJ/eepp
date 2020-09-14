#ifndef EE_SYSTEM_MD5_HPP
#define EE_SYSTEM_MD5_HPP

#include <eepp/core.hpp>
#include <eepp/system/iostream.hpp>
#include <vector>

namespace EE { namespace System {

class EE_API MD5 {
  public:
	struct Result {
		std::vector<Uint8> digest;

		std::string toHexString() { return MD5::hexDigest( digest ); }
	};

	/** @return Calculates the md5 hash from a stream */
	static Result fromStream( IOStream& stream );

	/** Calculates the md5 hash from a file */
	static Result fromFile( std::string path );

	/** Calculates the md5 hash from memory */
	static Result fromMemory( const Uint8* data, Uint64 size );

	/** Calculates the md5 hash from a string */
	static Result fromString( const std::string& str );

	/** Calculates the md5 hash from a string */
	static Result fromString( const String& str );

  protected:
	typedef unsigned int MD5_u32plus;

	struct Context {
		MD5_u32plus lo, hi;
		MD5_u32plus a, b, c, d;
		unsigned char buffer[64];
		MD5_u32plus block[16];
	};

	static const void* body( Context* ctx, const void* data, unsigned long size );

	static void init( Context* ctx );

	static void update( Context* ctx, const void* data, unsigned long size );

	static void final( unsigned char* result, Context* ctx );

	static std::string hexDigest( std::vector<Uint8>& digest );
};

}} // namespace EE::System

#endif
