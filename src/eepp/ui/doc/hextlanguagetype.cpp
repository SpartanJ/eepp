#include <array>
#include <sstream>

#include <eepp/ui/doc/hextlanguagetype.hpp>

namespace EE { namespace UI { namespace Doc {

// Checks if a character is a "word boundary".
inline bool isWordBoundary( char c ) {
	return !std::isalnum( static_cast<unsigned char>( c ) ) && c != '_';
}

// Finds a keyword as a whole word (e.g., finds "class" but not "subclass").
bool findStandaloneWord( const std::string& line, std::string_view keyword ) {
	size_t pos = 0;
	while ( ( pos = line.find( keyword, pos ) ) != std::string::npos ) {
		// Check character before the keyword
		bool preBoundary = ( pos == 0 ) || isWordBoundary( line[pos - 1] );

		// Check character after the keyword
		size_t endPos = pos + keyword.length();
		bool postBoundary = ( endPos == line.length() ) || isWordBoundary( line[endPos] );

		if ( preBoundary && postBoundary ) {
			return true; // Found a standalone occurrence
		}
		pos = endPos; // Continue search after this instance
	}
	return false;
}

/**
 * @brief Analyzes the content of a header file to determine its programming language.
 *
 * This function scans a string buffer for language-specific keywords and syntax
 * to distinguish between C, C++, Objective-C, and Objective-C++. It uses a
 * fast, heuristic-based approach optimized with std::array and std::string_view.
 *
 * @param buffer A std::string containing the source code from the .h file.
 * @return HExtLanguageType The detected language.
 */
HExtLanguageType HExtLanguageTypeHelper::detectLanguage( const std::string& buffer ) {
	bool hasCppFeature = false;
	bool hasObjcFeature = false;

	// Objective-C features
	static constexpr std::array<std::string_view, 6> objcKeywords = {
		"@interface", "@protocol", "@property", "@implementation", "@end", "@class" };
	static constexpr std::string_view objcDirective = "#import";

	// C++ features
	static constexpr std::array<std::string_view, 9> cppStandaloneKeywords = {
		"class", "namespace", "template", "virtual",  "override",
		"final", "public",	  "private",  "protected" };
	static constexpr std::array<std::string_view, 2> cppTokens = { "::", "template<" };

	std::stringstream ss( buffer );
	std::string line;
	bool inMultilineComment = false;

	while ( std::getline( ss, line ) ) {
		std::string processedLine = line;

		if ( inMultilineComment ) {
			size_t endCommentPos = processedLine.find( "*/" );
			if ( endCommentPos != std::string::npos ) {
				processedLine = processedLine.substr( endCommentPos + 2 );
				inMultilineComment = false;
			} else {
				continue;
			}
		}

		size_t startCommentPos = processedLine.find( "/*" );
		if ( startCommentPos != std::string::npos ) {
			size_t endCommentPos = processedLine.find( "*/", startCommentPos );
			if ( endCommentPos != std::string::npos ) {
				processedLine.erase( startCommentPos, endCommentPos - startCommentPos + 2 );
			} else {
				inMultilineComment = true;
				processedLine = processedLine.substr( 0, startCommentPos );
			}
		}

		size_t commentPos = processedLine.find( "//" );
		if ( commentPos != std::string::npos ) {
			processedLine = processedLine.substr( 0, commentPos );
		}

		if ( !hasObjcFeature ) {
			std::string_view lineView = processedLine;
			lineView.remove_prefix(
				std::min( lineView.find_first_not_of( " \t" ), lineView.size() ) );
			if ( lineView.rfind( objcDirective, 0 ) == 0 ) {
				hasObjcFeature = true;
			} else {
				for ( const auto& keyword : objcKeywords ) {
					if ( processedLine.find( keyword ) != std::string::npos ) {
						hasObjcFeature = true;
						break;
					}
				}
			}
		}

		if ( !hasCppFeature ) {
			for ( const auto& keyword : cppStandaloneKeywords ) {
				if ( findStandaloneWord( processedLine, keyword ) ) {
					hasCppFeature = true;
					break;
				}
			}
			if ( !hasCppFeature ) {
				for ( const auto& token : cppTokens ) {
					if ( processedLine.find( token ) != std::string::npos ) {
						hasCppFeature = true;
						break;
					}
				}
			}
			if ( !hasCppFeature ) {
				size_t ampPos = processedLine.find( '&' );
				if ( ampPos != std::string::npos && ampPos + 1 < processedLine.length() &&
					 processedLine[ampPos + 1] != '&' ) {
					hasCppFeature = true;
				}
			}
		}

		if ( hasCppFeature && hasObjcFeature )
			break;
	}

	if ( hasCppFeature && hasObjcFeature )
		return HExtLanguageType::ObjectiveCPP;

	if ( hasObjcFeature )
		return HExtLanguageType::ObjectiveC;

	if ( hasCppFeature )
		return HExtLanguageType::CPP;

	return HExtLanguageType::C;
}

std::string HExtLanguageTypeHelper::toString( HExtLanguageType langType ) {
	switch ( langType ) {
		case HExtLanguageType::AutoDetect:
			return "autodetect";
		case HExtLanguageType::C:
			return "c";
		case HExtLanguageType::CPP:
			return "cpp";
		case HExtLanguageType::ObjectiveC:
			return "objective-c";
		case HExtLanguageType::ObjectiveCPP:
			return "objective-cpp";
	}
	return "autodetect";
}

HExtLanguageType HExtLanguageTypeHelper::fromString( const std::string& langType ) {
	if ( langType == "c" )
		return HExtLanguageType::C;
	if ( langType == "cpp" )
		return HExtLanguageType::CPP;
	if ( langType == "objective-c" )
		return HExtLanguageType::ObjectiveC;
	if ( langType == "objective-cpp" )
		return HExtLanguageType::ObjectiveCPP;
	return HExtLanguageType::AutoDetect;
}

}}} // namespace EE::UI::Doc
