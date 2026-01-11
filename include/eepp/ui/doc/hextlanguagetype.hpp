#pragma once
#include <eepp/config.hpp>
#include <string>

namespace EE { namespace UI { namespace Doc {

enum class HExtLanguageType { AutoDetect, C, CPP, ObjectiveC, ObjectiveCPP };

struct EE_API HExtLanguageTypeHelper {
	static HExtLanguageType detectLanguage( std::string_view buffer );

	static std::string toString( HExtLanguageType langType );

	static HExtLanguageType fromString( const std::string& langType );
};

}}} // namespace EE::UI::Doc
