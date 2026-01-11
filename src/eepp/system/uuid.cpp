#include <eepp/system/uuid.hpp>

#include <random>

namespace EE { namespace System {

namespace {

static inline uint64_t hexToUint64( const std::string& hex ) {
	uint64_t result = 0;
	for ( char c : hex ) {
		result <<= 4;
		if ( c >= '0' && c <= '9' ) {
			result |= ( c - '0' );
		} else if ( c >= 'a' && c <= 'f' ) {
			result |= ( c - 'a' + 10 );
		} else {
			return 0;
		}
	}
	return result;
}

static inline void byteToHex( uint8_t byte, char* dest ) {
	static const char* hexDigits = "0123456789abcdef";
	dest[0] = hexDigits[( byte >> 4 ) & 0xF];
	dest[1] = hexDigits[byte & 0xF];
}

} // namespace

UUID::UUID() {
	refresh();
}

UUID::UUID( uint64_t high, uint64_t low ) : mHigh( high ), mLow( low ) {}

UUID::UUID( bool autocreate ) {
	if ( autocreate )
		refresh();
}

void UUID::refresh() {
	// Thread-local random number generator for performance in multi-threaded environments
	thread_local static std::random_device rd;
	thread_local static std::mt19937_64 gen( rd() );

	// Generate two 64-bit random numbers
	mHigh = gen();
	mLow = gen();

	// Set version (4) in bits 48-51 of the UUID (bits 12-15 of high_)
	// Clear bits 12-15 and set to 0b0100 (4 in hex)
	mHigh = ( mHigh & ~( (uint64_t)0xF << 12 ) ) | ( (uint64_t)0x4 << 12 );

	// Set variant (10) in bits 64-65 of the UUID (bits 63-62 of low_)
	// Clear bits 63-62 and set to 0b10
	mLow = ( mLow & ~( (uint64_t)0x3 << 62 ) ) | ( (uint64_t)0x2 << 62 );
}

std::optional<UUID> UUID::fromString( const std::string_view& uuidStr ) {
	// Check length (36 characters including hyphens)
	if ( uuidStr.length() != 36 )
		return {};

	// Verify hyphen positions
	if ( uuidStr[8] != '-' || uuidStr[13] != '-' || uuidStr[18] != '-' || uuidStr[23] != '-' )
		return {};

	// Verify version (must be '4' for UUIDv4)
	if ( uuidStr[14] != '4' )
		return {};

	// Verify variant (must be '8', '9', 'a', or 'b')
	char variantChar = uuidStr[19];
	if ( !( ( variantChar >= '8' && variantChar <= '9' ) ||
			( variantChar >= 'a' && variantChar <= 'b' ) ) )
		return {};

	// Remove hyphens and convert to lowercase
	std::string hexStr;
	for ( char c : uuidStr ) {
		if ( c != '-' ) {
			if ( !std::isxdigit( c ) )
				return {};
			hexStr += std::tolower( c );
		}
	}

	// Verify length after removing hyphens (32 hex chars)
	if ( hexStr.length() != 32 )
		return {};

	// Convert to 128-bit binary (two 64-bit parts)
	uint64_t high_ = hexToUint64( hexStr.substr( 0, 16 ) );
	uint64_t low_ = hexToUint64( hexStr.substr( 16, 16 ) );

	return UUID{ high_, low_ };
}

std::string UUID::toString() const {
	char buf[36]; // 32 hex digits + 4 hyphens = 36 characters
	int pos = 0;
	uint64_t parts[2] = { mHigh, mLow };
	int byteIndex = 0;

	// Process all 16 bytes, inserting hyphens at the correct positions
	for ( int part = 0; part < 2; ++part ) {
		uint64_t val = parts[part];
		for ( int i = 56; i >= 0; i -= 8 ) {
			byteToHex( ( val >> i ) & 0xFF, buf + pos );
			pos += 2;
			byteIndex++;
			// Insert hyphens after bytes 4, 6, 8, and 10 (positions 8, 13, 18, 23)
			if ( byteIndex == 4 || byteIndex == 6 || byteIndex == 8 || byteIndex == 10 ) {
				buf[pos++] = '-';
			}
		}
	}
	return std::string( buf, 36 );
}

bool UUID::isInitialized() const {
	return mLow != 0 || mHigh != 0;
}

}} // namespace EE::System
