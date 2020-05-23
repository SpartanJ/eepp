#include <eepp/core/memorymanager.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>

namespace EE { namespace UI { namespace Doc {

SyntaxDefinition::SyntaxDefinition() {}

SyntaxDefinition::SyntaxDefinition( const std::vector<std::string>& files,
									const std::vector<SyntaxPattern>& patterns,
									const std::unordered_map<std::string, std::string>& symbols,
									const std::string& comment ) :
	mFiles( files ), mPatterns( patterns ), mSymbols( symbols ), mComment( comment ) {}

const std::vector<std::string>& SyntaxDefinition::getFiles() const {
	return mFiles;
}

const std::vector<SyntaxPattern>& SyntaxDefinition::getPatterns() const {
	return mPatterns;
}

const std::string& SyntaxDefinition::getComment() const {
	return mComment;
}

const std::unordered_map<std::string, std::string>& SyntaxDefinition::getSymbols() const {
	return mSymbols;
}

std::string SyntaxDefinition::getSymbol( const std::string& symbol ) const {
	auto it = mSymbols.find( symbol );
	if ( it != mSymbols.end() )
		return symbol;
	return "";
}

}}} // namespace EE::UI::Doc
