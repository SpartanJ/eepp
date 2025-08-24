#include <array>

#include <eepp/core/string.hpp>
#include <eepp/ui/doc/hextlanguagetype.hpp>

namespace EE { namespace UI { namespace Doc {

// Checks if a character is a "word boundary".
inline bool isWordBoundary( char c ) {
	return !std::isalnum( static_cast<unsigned char>( c ) ) && c != '_';
}

// Finds a keyword as a whole word (e.g., finds "class" but not "subclass").
bool findStandaloneWord( std::string_view line, std::string_view keyword ) {
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
 * @param buffer A std::string_view containing the source code from the .h file.
 * @return HExtLanguageType The detected language.
 */
HExtLanguageType HExtLanguageTypeHelper::detectLanguage( std::string_view buffer ) {
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

	bool inMultilineComment = false;

	String::readBySeparatorStoppable( buffer, [&]( std::string_view line ) {
		std::string_view processedLine = line;

		if ( inMultilineComment ) {
			size_t endCommentPos = processedLine.find( "*/" );
			if ( endCommentPos != std::string::npos ) {
				processedLine = processedLine.substr( endCommentPos + 2 );
				inMultilineComment = false;
			} else {
				return false; // Skip the line, still in multiline comment
			}
		}

		// Handle multiline comments (/* ... */)
		while ( true ) {
			size_t startCommentPos = processedLine.find( "/*" );
			if ( startCommentPos == std::string_view::npos ) {
				break; // No more multiline comments
			}
			size_t endCommentPos = processedLine.find( "*/", startCommentPos );
			if ( endCommentPos != std::string_view::npos ) {
				// Skip the comment by creating a view after the comment
				processedLine = processedLine.substr( endCommentPos + 2 );
			} else {
				// Multiline comment extends to next line
				inMultilineComment = true;
				processedLine = processedLine.substr( 0, startCommentPos );
				break;
			}
		}

		// Handle single-line comments (//)
		size_t commentPos = processedLine.find( "//" );
		if ( commentPos != std::string_view::npos ) {
			processedLine = processedLine.substr( 0, commentPos );
		}

		if ( !hasObjcFeature ) {
			std::string_view lineView = processedLine;
			lineView.remove_prefix(
				std::min( lineView.find_first_not_of( " \t" ), lineView.size() ) );
			if ( lineView.starts_with( objcDirective ) ) {
				hasObjcFeature = true;
			} else {
				for ( const auto& keyword : objcKeywords ) {
					if ( processedLine.find( keyword ) != std::string_view::npos ) {
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
					if ( processedLine.find( token ) != std::string_view::npos ) {
						hasCppFeature = true;
						break;
					}
				}
			}
			if ( !hasCppFeature ) {
				size_t ampPos = processedLine.find( '&' );
				if ( ampPos != std::string_view::npos && ampPos + 1 < processedLine.length() &&
					 processedLine[ampPos + 1] != '&' ) {
					hasCppFeature = true;
				}
			}
		}

		return hasCppFeature && hasObjcFeature; // Stop if both features are found
	} );

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
