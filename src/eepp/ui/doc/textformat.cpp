#include <eepp/core/debug.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/textformat.hpp>

namespace EE { namespace UI { namespace Doc {

static constexpr Uint32 NumBytesForAutodetect = 16000;

// Adapted from plywood https://preshing.com/20200727/automatically-detecting-text-encodings-in-cpp/
// MIT Licensed
struct TextDecodeResult {
	enum class Status : Uint8 {
		Truncated, // std::string_view wasn't long enough to read a valid point. A (invalid) point
				   // may be available anyway, such as when flushing the last few bytes of a UTF-8
				   // file.
		Invalid, // Invalid byte sequence was encountered. Such sequences are typically decoded one
				 // code unit at a time.
		Valid,
	};

	// (point >= 0) if and only if (numBytes > 0), which means that a code point is available to
	// read (even if status is Invalid or Truncated).
	Int32 point = -1;
	Status status = Status::Truncated;
	Uint8 numBytes = 0;
};

//-------------------------------------------------------------------
// Enc_Bytes
//-------------------------------------------------------------------
struct Enc_Bytes {
	static inline TextDecodeResult decodePoint( std::string_view view ) {
		return view.empty()
				   ? TextDecodeResult{}
				   : TextDecodeResult{ (Uint8)view[0], TextDecodeResult::Status::Valid, 1 };
	}
};

//-------------------------------------------------------------------
// UTF8
//-------------------------------------------------------------------
struct UTF8 {
	static TextDecodeResult decodePointSlowPath( std::string_view view );

	static inline TextDecodeResult decodePoint( std::string_view view ) {
		if ( view.size() > 0 ) {
			Uint8 first = view[0];
			if ( first < 0x80 ) {
				return { first, TextDecodeResult::Status::Valid, 1 };
			}
		}
		return decodePointSlowPath( view );
	}
};

//-------------------------------------------------------------------
// UTF16
//-------------------------------------------------------------------
template <bool BigEndian> struct UTF16 {
	static inline Uint16 getUnit( const char* src ) {
		if constexpr ( BigEndian ) {
			return ( Uint16( Uint8( src[0] ) ) << 8 ) | Uint8( src[1] );
		} else {
			return Uint8( src[0] ) | ( Uint16( Uint8( src[1] ) ) << 8 );
		}
	}

	static inline TextDecodeResult decodePoint( std::string_view view ) {
		if ( view.size() < 2 ) {
			return {};
		}
		Uint16 first = getUnit( view.data() );
		auto status = TextDecodeResult::Status::Invalid;
		if ( first >= 0xd800 && first < 0xdc00 ) {
			if ( view.size() < 4 ) {
				status = TextDecodeResult::Status::Truncated;
			} else {
				Uint16 second = getUnit( view.data() + 2 );
				if ( second >= 0xdc00 && second < 0xe000 ) {
					Uint32 value = 0x10000 + ( ( first - 0xd800 ) << 10 ) + ( second - 0xdc00 );
					return { (Int32)value, TextDecodeResult::Status::Valid, 4 };
				}
			}
		} else if ( !( first >= 0xdc00 && first < 0xe000 ) ) {
			status = TextDecodeResult::Status::Valid;
		}
		return { first, status, 2 };
	}
};

using UTF16_LE = UTF16<false>;
using UTF16_BE = UTF16<true>;

//-------------------------------------------------------------------
// Shift JIS
//-------------------------------------------------------------------
struct ShiftJIS {
	static inline Uint16 getUnit( const char* src ) {
		return Uint8( src[0] ) | ( Uint16( Uint8( src[1] ) ) << 8 );
	}

	static inline TextDecodeResult decodePoint( std::string_view view ) {
		// Shift JIS ranges for single-byte and double-byte characters
		static constexpr std::pair<unsigned char, unsigned char> firstByteRange1( 0x81, 0x9F );
		static constexpr std::pair<unsigned char, unsigned char> firstByteRange2( 0xE0, 0xEF );
		static constexpr std::pair<unsigned char, unsigned char> secondByteRange1( 0x40, 0x7E );
		static constexpr std::pair<unsigned char, unsigned char> secondByteRange2( 0x80, 0xFC );

		if ( view.size() == 0 )
			return {};

		Uint8 first = view[0];
		if ( first < 0x7F )
			return { first, TextDecodeResult::Status::Valid, 1 };

		if ( view.size() < 2 &&
			 ( ( first >= secondByteRange1.first && first <= secondByteRange1.second ) ||
			   ( first >= secondByteRange2.first && first <= secondByteRange2.second ) ) ) {
			return { first, TextDecodeResult::Status::Valid, 1 };
		}

		Uint8 second = view[1];

		if ( ( ( first >= firstByteRange1.first && first <= firstByteRange1.second ) ||
			   ( first >= firstByteRange2.first && first <= firstByteRange2.second ) ) &&
			 ( ( second >= secondByteRange1.first && second <= secondByteRange1.second ) ||
			   ( second >= secondByteRange2.first && second <= secondByteRange2.second ) ) ) {
			return { getUnit( view.data() ), TextDecodeResult::Status::Valid, 2 };
		}

		return { first, TextDecodeResult::Status::Invalid, 1 };
	}
};

//-------------------------------------------------------------------
// UTF8
//-------------------------------------------------------------------
TextDecodeResult UTF8::decodePointSlowPath( std::string_view view ) {
	if ( view.size() == 0 ) {
		return {};
	}

	TextDecodeResult result; // Default status is Truncated
	Uint32 value = 0;
	const Uint8* bytes = (const Uint8*)view.data();
	Uint8 first = *bytes++;
	switch ( ( first >> 3 ) & 0xf ) {
		case 0b1000:
		case 0b1001:
		case 0b1010:
		case 0b1011: {
			if ( view.size() >= 2 ) {
				if ( ( bytes[0] & 0xc0 ) == 0x80 ) {
					result.numBytes = 2;
					value = first & 0x1f;
					goto consume1Byte;
				}
				result.status = TextDecodeResult::Status::Invalid;
			}
			break;
		}

		case 0b1100:
		case 0b1101: {
			if ( view.size() >= 3 ) {
				if ( ( bytes[0] & 0xc0 ) == 0x80 && ( bytes[1] & 0xc0 ) == 0x80 ) {
					result.numBytes = 3;
					value = first & 0xf;
					goto consume2Bytes;
				}
				result.status = TextDecodeResult::Status::Invalid;
			}
			break;
		}

		case 0b1110: {
			if ( view.size() >= 4 ) {
				if ( ( bytes[0] & 0xc0 ) == 0x80 && ( bytes[1] & 0xc0 ) == 0x80 &&
					 ( bytes[2] & 0xc0 ) == 0x80 ) {
					result.numBytes = 4;
					value = first & 0x7;
					goto consume3Bytes;
				}
				result.status = TextDecodeResult::Status::Invalid;
			}
			break;
		}

		default:
			break;
	}

	// Bad encoding; consume just one byte
	// Invalid/truncated status has already been set
	result.point = first;
	result.numBytes = 1;
	return result;

consume3Bytes:
	value = ( value << 6 ) | ( *bytes & 0x3f );
	bytes++;
consume2Bytes:
	value = ( value << 6 ) | ( *bytes & 0x3f );
	bytes++;
consume1Byte:
	value = ( value << 6 ) | ( *bytes & 0x3f );
	result.point = value;
	result.status = TextDecodeResult::Status::Valid;
	return result;
}

//-------------------------------------------------------------------
// TextEncoding helper objects
//-------------------------------------------------------------------
struct TextEncoding {
	TextDecodeResult ( *decodePoint )( std::string_view view ) = nullptr;
	Uint32 unitSize = 0;

	template <typename> struct Wrapper;
	template <typename Enc> inline static const TextEncoding* get() {
		return &TextEncoding::Wrapper<Enc>::Instance;
	}
};

template <> struct TextEncoding::Wrapper<Enc_Bytes> {
	static TextEncoding Instance;
};
template <> struct TextEncoding::Wrapper<UTF8> {
	static TextEncoding Instance;
};
template <> struct TextEncoding::Wrapper<UTF16_LE> {
	static TextEncoding Instance;
};
template <> struct TextEncoding::Wrapper<UTF16_BE> {
	static TextEncoding Instance;
};
template <> struct TextEncoding::Wrapper<ShiftJIS> {
	static TextEncoding Instance;
};

//-------------------------------------------------------------------
// TextEncoding (indirect through function vectors)
//-------------------------------------------------------------------
TextEncoding TextEncoding::Wrapper<Enc_Bytes>::Instance = {
	&Enc_Bytes::decodePoint,
	1,
};

TextEncoding TextEncoding::Wrapper<UTF8>::Instance = {
	&UTF8::decodePoint,
	1,
};

TextEncoding TextEncoding::Wrapper<UTF16_LE>::Instance = {
	&UTF16_LE::decodePoint,
	2,
};

TextEncoding TextEncoding::Wrapper<UTF16_BE>::Instance = {
	&UTF16_BE::decodePoint,
	2,
};

TextEncoding TextEncoding::Wrapper<ShiftJIS>::Instance = {
	&ShiftJIS::decodePoint,
	1,
};

const TextEncoding* encodingFromEnum( TextFormat::Encoding enc ) {
	switch ( enc ) {
		default:
			eeASSERT( 0 );
		case TextFormat::Encoding::Latin1:
			return TextEncoding::get<Enc_Bytes>();
		case TextFormat::Encoding::UTF8:
			return TextEncoding::get<UTF8>();
		case TextFormat::Encoding::UTF16BE:
			return TextEncoding::get<UTF16<true>>();
		case TextFormat::Encoding::UTF16LE:
			return TextEncoding::get<UTF16<false>>();
	}
};

struct TextFileStats {
	Uint32 numPoints = 0;
	Uint32 numValidPoints = 0;
	Uint32 totalPointValue = 0; // This value won't be accurate if byte encoding is detected
	Uint32 numLines = 0;
	Uint32 numCRLF = 0;
	Uint32 numControl = 0; // non-whitespace points < 32, including nulls
	Uint32 numNull = 0;
	Uint32 numPlainAscii = 0; // includes whitespace, excludes control characters < 32
	Uint32 numWhitespace = 0;
	Uint32 numExtended = 0;
	Uint32 num16bytes = 0;
	float ooNumPoints = 0.f;
	float score = 0.f;
	bool count16b{ false };

	Uint32 numInvalidPoints() const { return numPoints - numValidPoints; }

	TextFormat::LineEnding getNewLineType() const {
		eeASSERT( numCRLF <= numLines );
		if ( numCRLF == 0 || numCRLF * 2 < numLines ) {
			return TextFormat::LineEnding::LF;
		} else {
			return TextFormat::LineEnding::CRLF;
		}
	}

	void calcScore() {
		score = ( ( 2.5f * numWhitespace + numPlainAscii - 100.f * numInvalidPoints() -
					50.f * numControl + 5.f * numExtended + 2.5f * num16bytes ) *
				  ( count16b ? ( num16bytes > 0 ? 1 : 0 ) : 1 ) ) *
				ooNumPoints;
	}

	bool allValidPoints() const { return numPoints == numValidPoints; }

	float getScore() {
		calcScore();
		return score;
	}
};

static Uint32 scanTextFile( TextFileStats& stats, IOStream& ins, const TextEncoding* encoding,
							Uint32 maxBytes ) {
	if ( encoding == nullptr )
		return 0;
	bool prevWasCR = false;
	Uint32 numBytes = 0;
	ins.seek( 0 );
	char buf[4];
	while ( numBytes < maxBytes ) {
		size_t read = ins.read( buf, 4 );
		if ( 0 == read )
			break;

		TextDecodeResult decoded = encoding->decodePoint( std::string_view{ buf, read } );
		if ( decoded.status == TextDecodeResult::Status::Truncated && decoded.numBytes == 0 )
			break; // EOF/error

		numBytes += decoded.numBytes;
		ins.seek( numBytes );

		stats.numPoints++;
		if ( decoded.status == TextDecodeResult::Status::Valid ) {
			eeASSERT( decoded.point >= 0 && decoded.numBytes > 0 );
			stats.numValidPoints++;
			stats.totalPointValue += decoded.point;
			if ( decoded.point < 32 ) {
				if ( decoded.point == '\n' ) {
					stats.numPlainAscii++;
					stats.numLines++;
					stats.numWhitespace++;
					if ( prevWasCR ) {
						stats.numCRLF++;
					}
				} else if ( decoded.point == '\t' ) {
					stats.numPlainAscii++;
					stats.numWhitespace++;
				} else if ( decoded.point == '\r' ) {
					stats.numPlainAscii++;
				} else {
					stats.numControl++;
					if ( decoded.point == 0 ) {
						stats.numNull++;
					}
				}
			} else if ( decoded.point < 127 ) {
				stats.numPlainAscii++;
				if ( decoded.point == ' ' ) {
					stats.numWhitespace++;
				}
			} else if ( decoded.point >= 65536 ) {
				stats.numExtended++;
			} else if ( stats.count16b && decoded.point >= 0x8140 ) {
				stats.num16bytes++;
			}
		}
		prevWasCR = ( decoded.point == '\r' );
	}
	if ( stats.numPoints > 0 ) {
		stats.ooNumPoints = 1.f / stats.numPoints;
	}
	stats.calcScore();
	return numBytes;
}

TextFormat guessFileEncoding( IOStream& ins ) {
	auto start = ins.tell();
	ScopedOp op( [] {}, [&start, &ins] { ins.seek( start ); } );
	TextFileStats stats8;

	// Try UTF8 first:
	Uint32 numBytesRead =
		scanTextFile( stats8, ins, TextEncoding::get<UTF8>(), NumBytesForAutodetect );
	if ( numBytesRead == 0 )
		return { TextFormat::Encoding::UTF8, stats8.getNewLineType(), false };

	ins.seek( 0 );
	if ( stats8.numInvalidPoints() == 0 && stats8.numControl == 0 ) {
		// No UTF-8 encoding errors, and no weird control characters/nulls. Pick UTF-8.
		return { TextFormat::Encoding::UTF8, stats8.getNewLineType(), false };
	}

	// If more than 20% of the high bytes in UTF-8 are encoding errors, reinterpret UTF-8 as just
	// bytes.
	TextFormat::Encoding encoding8 = TextFormat::Encoding::UTF8;
	{
		Uint32 numHighBytes = numBytesRead - stats8.numPlainAscii - stats8.numControl;
		if ( stats8.numInvalidPoints() >= numHighBytes * 0.2f ) {
			// Too many UTF-8 errors. Consider it bytes.
			encoding8 = TextFormat::Encoding::Latin1;
			stats8.numPoints = numBytesRead;
			stats8.numValidPoints = numBytesRead;
		}
	}

	// Examine both UTF16 endianness:
	TextFileStats stats16_le;
	scanTextFile( stats16_le, ins, TextEncoding::get<UTF16_LE>(), NumBytesForAutodetect );
	ins.seek( 0 );

	TextFileStats stats16_be;
	scanTextFile( stats16_be, ins, TextEncoding::get<UTF16_BE>(), NumBytesForAutodetect );
	ins.seek( 0 );

	// Choose the better UTF16 candidate:
	TextFileStats* stats = &stats16_le;
	TextFormat::Encoding encoding = TextFormat::Encoding::UTF16LE;
	if ( stats16_be.getScore() > stats16_le.getScore() ) {
		stats = &stats16_be;
		encoding = TextFormat::Encoding::UTF16BE;
	}

	TextFileStats statsShiftJIS;
	statsShiftJIS.count16b = true;
	scanTextFile( statsShiftJIS, ins, TextEncoding::get<ShiftJIS>(), NumBytesForAutodetect );
	ins.seek( 0 );

	if ( statsShiftJIS.allValidPoints() && statsShiftJIS.num16bytes &&
		 statsShiftJIS.getScore() > stats->getScore() ) {
		stats = &statsShiftJIS;
		encoding = TextFormat::Encoding::Shift_JIS;
	}

	// Choose between the UTF16 and 8-bit encoding:
	if ( stats8.getScore() >= stats->getScore() ) {
		stats = &stats8;
		encoding = encoding8;
	}

	// Return best guess
	return { encoding, stats->getNewLineType(), false };
}

TextFormat TextFormat::autodetect( IOStream& ins ) {
	const auto readByte = [&ins]() -> Uint8 {
		Uint8 byte;
		ins.read( (char*)&byte, 1 );
		return byte;
	};
	TextFormat tff;
	auto start = ins.tell();
	auto size = ins.getSize();
	if ( size >= 2 ) {
		Uint8 h[3] = { 0 };
		h[0] = readByte();
		h[1] = readByte();
		if ( h[0] == 0xef && h[1] == 0xbb && size >= 3 ) {
			h[2] = readByte();
			if ( h[2] == 0xbf ) {
				tff.encoding = TextFormat::Encoding::UTF8;
				tff.bom = true;
			}
		} else if ( h[0] == 0xfe && h[1] == 0xff ) {
			tff.encoding = TextFormat::Encoding::UTF16BE;
			tff.bom = true;
		} else if ( h[0] == 0xff && h[1] == 0xfe ) {
			tff.encoding = TextFormat::Encoding::UTF16LE;
			tff.bom = true;
		}
		ins.seek( start );
	}
	if ( !tff.bom ) {
		tff = guessFileEncoding( ins );
	} else {
		// Detect LF or CRLF
		TextFileStats stats;
		scanTextFile( stats, ins, encodingFromEnum( tff.encoding ), NumBytesForAutodetect );
		tff.newLine = stats.getNewLineType();
	}
	return tff;
}

std::string TextFormat::lineEndingToString( const LineEnding& le ) {
	switch ( le ) {
		case TextFormat::LineEnding::CRLF:
			return "CRLF";
		case TextFormat::LineEnding::CR:
			return "CR";
		case TextFormat::LineEnding::LF:
		default:
			return "LF";
	}
}

TextFormat::LineEnding TextFormat::stringToLineEnding( const std::string& str ) {
	if ( "CR" == str )
		return TextFormat::LineEnding::CR;
	if ( "CRLF" == str )
		return TextFormat::LineEnding::CRLF;
	return TextFormat::LineEnding::LF;
}

TextFormat::Encoding TextFormat::encodingFromString( const std::string& str ) {
	switch ( String::hash( str ) ) {
		case static_cast<String::HashType>( TextFormat::Encoding::UTF16LE ):
			return TextFormat::Encoding::UTF16LE;
		case static_cast<String::HashType>( TextFormat::Encoding::UTF16BE ):
			return TextFormat::Encoding::UTF16BE;
		case static_cast<String::HashType>( TextFormat::Encoding::Latin1 ):
			return TextFormat::Encoding::Latin1;
		case static_cast<String::HashType>( TextFormat::Encoding::Shift_JIS ):
			return TextFormat::Encoding::Shift_JIS;
		case static_cast<String::HashType>( TextFormat::Encoding::UTF8 ):
		default:
			return TextFormat::Encoding::UTF8;
	}
}

std::string TextFormat::encodingToString( TextFormat::Encoding enc ) {
	switch ( enc ) {
		case TextFormat::Encoding::UTF16LE:
			return "UTF-16 LE";
		case TextFormat::Encoding::UTF16BE:
			return "UTF-16 BE";
		case TextFormat::Encoding::Latin1:
			return "ISO-8859-1";
		case TextFormat::Encoding::Shift_JIS:
			return "Shift-JIS";
		case TextFormat::Encoding::UTF8:
		default:
			break;
	}
	return "UTF-8";
}

std::vector<std::pair<TextFormat::Encoding, std::string>> TextFormat::encodings() {
	std::vector<std::pair<TextFormat::Encoding, std::string>> encs;
	encs.emplace_back( Encoding::UTF8, encodingToString( Encoding::UTF8 ) );
	encs.emplace_back( Encoding::UTF16BE, encodingToString( Encoding::UTF16BE ) );
	encs.emplace_back( Encoding::UTF16LE, encodingToString( Encoding::UTF16LE ) );
	encs.emplace_back( Encoding::Latin1, encodingToString( Encoding::Latin1 ) );
	encs.emplace_back( Encoding::Shift_JIS, encodingToString( Encoding::Shift_JIS ) );
	return encs;
}

}}} // namespace EE::UI::Doc
