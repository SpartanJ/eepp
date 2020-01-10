#ifndef EE_SYSTEM_BASE64_HPP
#define EE_SYSTEM_BASE64_HPP

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <eepp/config.hpp>
#include <string>

namespace EE { namespace System {

class EE_API Base64 {
  public:
	/** Encode binary data into base64 digits with MIME style === pads
	**  @return The final length of the output */
	static int encode( size_t in_len, const unsigned char* in, size_t out_len, char* out );

	/** Decode base64 digits with MIME style === pads into binary data
	**  @return The final length of the output */
	static int decode( size_t in_len, const char* in, size_t out_len, unsigned char* out );

	/** Encodes a string into a base64 string
	**  @return True if encoding was successful */
	static bool encode( const std::string& in, std::string& out );

	/** Decodes a base64 string to a string
	**  @return True if encoding was successful */
	static bool decode( const std::string& in, std::string& out );

	/** @return A safe encoding output length for an input of the length indicated */
	static inline int encodeSafeOutLen( size_t in_len ) { return in_len / 3 * 4 + 4 + 1; }

	/** @return A safe decoding output length for an input of the length indicated */
	static inline int decodeSafeOutLen( size_t in_len ) { return in_len / 4 * 3 + 1; }
};

}} // namespace EE::System

#endif
