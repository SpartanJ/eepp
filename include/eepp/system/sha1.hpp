#ifndef EE_SYSTEM_SHA1_HPP
#define EE_SYSTEM_SHA1_HPP

#include <array>
#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/iostream.hpp>

namespace EE { namespace System {

/** @brief SHA-1 (FIPS 180-4) message digest.
 *
 * SHA-1 is not collision resistant: use it for identifiers and interoperability (for example
 * OpenSSH's %C token or hashed known_hosts names), never for new security-sensitive designs.
 */
class EE_API SHA1 {
  public:
	using Digest = std::array<Uint8, 20>;

	struct Result {
		Digest digest;

		std::string toHexString() const { return SHA1::hexDigest( digest ); }

		bool operator==( const Result& other ) const { return digest == other.digest; }

		bool operator!=( const Result& other ) const { return digest != other.digest; }
	};

	struct Context {
		Uint32 state[5];
		/** Total message length in bytes. */
		Uint64 length;
		unsigned char buffer[64];
	};

	/** @return Calculates the sha1 hash from a stream */
	static Result fromStream( IOStream& stream );

	/** Calculates the sha1 hash from a file */
	static Result fromFile( std::string path );

	/** Calculates the sha1 hash from memory */
	static Result fromMemory( const Uint8* data, Uint64 size );

	/** Calculates the sha1 hash from a string */
	static Result fromString( const std::string& str );

	/** Calculates the sha1 hash from a string */
	static Result fromString( const String& str );

	static void init( Context& ctx );

	static void update( Context& ctx, const void* data, unsigned long size );

	static void final( Digest& result, Context& ctx );

	static Result result( Context& ctx );

	static std::string hexDigest( const Digest& digest );

  private:
	static void transform( Context& ctx, const unsigned char* block );
};

}} // namespace EE::System

#endif
