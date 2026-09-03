#ifndef EE_SYSTEM_BASE64_HPP
#define EE_SYSTEM_BASE64_HPP

#include <cstdio>
#include <cstdlib>
#include <eepp/config.hpp>
#include <string>
#include <string_view>

namespace EE { namespace System {

class EE_API Base64 {
  public:
	enum class DecodeMode {
		AllowWhitespace,
		NoWhitespace,
	};

	/** Encode binary data into base64 digits with MIME style === pads
	**  @return The final length of the output */
	static size_t encode( size_t in_len, const unsigned char* in, size_t out_len, char* out );

	/** Decode base64 digits with MIME style === pads into binary data.
	**  Preserves the historical behavior of ignoring ASCII whitespace.
	**  @return The final length of the output */
	static size_t decode( size_t in_len, const char* in, size_t out_len, unsigned char* out );

	/** Decode base64 digits with MIME style === pads into binary data.
	**  DecodeMode::NoWhitespace avoids all whitespace handling and enables the fastest path.
	**  @return The final length of the output */
	static size_t decode( size_t in_len, const char* in, size_t out_len, unsigned char* out,
						  DecodeMode mode );

	/** Encodes a string into a base64 string
	**  @return True if encoding was successful */
	static bool encode( std::string_view in, std::string& out );

	/** Decodes a base64 string to a string, ignoring ASCII whitespace.
	**  @return The final length of the output, or size_t(-1) on truncation */
	static size_t decode( std::string_view in, std::string& out );

	/** Decodes a base64 string to a string.
	**  @return The final length of the output, or size_t(-1) on truncation */
	static size_t decode( std::string_view in, std::string& out, DecodeMode mode );

	/** @return A safe encoding output length for an input of the length indicated */
	static inline size_t encodeSafeOutLen( size_t in_len ) {
		return ( ( in_len + 2 ) / 3 ) * 4 + 1;
	}

	/** @return A safe decoding output length for an input of the length indicated */
	static inline size_t decodeSafeOutLen( size_t in_len ) {
		return ( ( in_len + 3 ) / 4 ) * 3 + 1;
	}
};

}} // namespace EE::System

#endif
