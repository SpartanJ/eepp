#ifndef EE_EXIF_TEST_UTILS_HPP
#define EE_EXIF_TEST_UTILS_HPP
#include <eepp/config.hpp>
#include <string>

namespace exiftest {

inline std::string tiff( EE::Uint16 orientation, bool little = true ) {
	std::string bytes( 26, '\0' );
	bytes[0] = bytes[1] = little ? 'I' : 'M';
	const auto put = [&]( size_t offset, EE::Uint32 value, size_t count ) {
		for ( size_t i = 0; i < count; ++i )
			bytes[offset + i] = char( value >> ( 8 * ( little ? i : count - 1 - i ) ) );
	};
	put( 2, 42, 2 );
	put( 4, 8, 4 );
	put( 8, 1, 2 );
	put( 10, 0x112, 2 );
	put( 12, 3, 2 );
	put( 14, 1, 4 );
	put( 18, orientation, 2 );
	return bytes;
}

inline std::string jpeg( const std::string& original, const std::string& metadata ) {
	const auto length = metadata.size() + 8;
	std::string bytes( "\xff\xd8\xff\xe1", 4 );
	bytes.push_back( char( length >> 8 ) );
	bytes.push_back( char( length ) );
	bytes.append( "Exif\0\0", 6 );
	bytes += metadata;
	bytes += original.substr( 2 );
	return bytes;
}

} // namespace exiftest
#endif
