#ifndef EE_UI_DOC_TEXTFORMAT_HPP
#define EE_UI_DOC_TEXTFORMAT_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/iostream.hpp>

using namespace EE;
using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class EE_API TextFormat {
  public:
	enum class Encoding : String::HashType {
		UTF8 = String::hash( "UTF-8" ),
		UTF16LE = String::hash( "UTF-16 LE" ),
		UTF16BE = String::hash( "UTF-16 BE" ),
		Latin1 = String::hash( "ISO-8859-1" ),
		Shift_JIS = String::hash( "Shift-JIS" ),
	};

	enum class LineEnding { LF, CRLF, CR };

	Encoding encoding = Encoding::UTF8;
	LineEnding newLine = LineEnding::LF;
	bool bom = false;

	static std::vector<std::pair<Encoding, std::string>> encodings();

	static std::string lineEndingToString( const TextFormat::LineEnding& le );

	static TextFormat::LineEnding stringToLineEnding( const std::string& str );

	static std::string encodingToString( TextFormat::Encoding enc );

	static Encoding encodingFromString( const std::string& str );

	static TextFormat autodetect( IOStream& ins );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_TEXTFORMAT_HPP
