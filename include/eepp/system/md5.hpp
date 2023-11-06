#ifndef EE_SYSTEM_MD5_HPP
#define EE_SYSTEM_MD5_HPP

#include <eepp/core.hpp>
#include <eepp/system/iostream.hpp>
#include <array>

namespace EE { namespace System {

class EE_API MD5 {
  public:
	using Digest = std::array<Uint8, 16>;

	struct Result {
		Digest digest;

		std::string toHexString() const { return MD5::hexDigest( digest ); }

		bool operator==( const Result& other ) { return digest == other.digest; }

		bool operator!=( const Result& other ) { return digest != other.digest; }
	};

	struct Context {
		Uint32 lo, hi;
		Uint32 a, b, c, d;
		unsigned char buffer[64];
		Uint32 block[16];
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

	static void init( Context& ctx );

	static void update( Context& ctx, const void* data, unsigned long size );

	static void final( Digest& result, Context& ctx );

	static Result result( Context& ctx );

	static std::string hexDigest( const Digest& digest );

  private:
	static const void* body( Context& ctx, const void* data, unsigned long size );
};

}} // namespace EE::System

#endif
