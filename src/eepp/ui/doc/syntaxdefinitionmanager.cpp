#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

SINGLETON_DECLARE_IMPLEMENTATION( SyntaxDefinitionManager )

SyntaxDefinitionManager::SyntaxDefinitionManager() {
	// Register some languages support.
	// XML - HTML
	add( SyntaxDefinition( {"xml", "html"}, {{{"<!%-%-", "%-%->"}, "comment"},
											 {{"%f[^>][^<]", "%f[<]"}, "normal"},
											 {{"\"", "\"", "\\"}, "string"},
											 {{"'", "'", "\\"}, "string"},
											 {{"0x[%da-fA-F]+"}, "number"},
											 {{"-?%d+[%d%.]*f?"}, "number"},
											 {{"-?%.?%d+f?"}, "number"},
											 {{"%f[^<]![%a_][%w_]*"}, "keyword2"},
											 {{"%f[^<][%a_][%w_]*"}, "function"},
											 {{"%f[^<]/[%a_][%w_]*"}, "function"},
											 {{"[%a_][%w_]*"}, "keyword"},
											 {{"[/<>=]"}, "operator"}} ) );
}

void SyntaxDefinitionManager::add( SyntaxDefinition&& syntaxStyle ) {
	mStyles.emplace_back( std::move( syntaxStyle ) );
}

const SyntaxDefinition PLAIN_STYLE = SyntaxDefinition();

const SyntaxDefinition& SyntaxDefinitionManager::getPlainStyle() const {
	return PLAIN_STYLE;
}

const SyntaxDefinition&
SyntaxDefinitionManager::getStyleByExtension( const std::string& filePath ) const {
	std::string extension( FileSystem::fileExtension( filePath ) );
	if ( !extension.empty() ) {
		for ( auto& syntax : mStyles ) {
			auto it = std::find( syntax.getFiles().begin(), syntax.getFiles().end(), extension );
			if ( it != syntax.getFiles().end() ) {
				return syntax;
			}
		}
	}
	return PLAIN_STYLE;
}

}}} // namespace EE::UI::Doc
