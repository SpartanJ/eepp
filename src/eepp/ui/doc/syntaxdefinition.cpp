#include <eepp/core/memorymanager.hpp>
#include <eepp/core/string.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>

namespace EE { namespace UI { namespace Doc {

SyntaxDefinition::SyntaxDefinition() {}

SyntaxDefinition::SyntaxDefinition( const std::string& languageName,
									const std::vector<std::string>& files,
									const std::vector<SyntaxPattern>& patterns,
									const std::unordered_map<std::string, std::string>& symbols,
									const std::string& comment,
									const std::vector<std::string> headers ) :
	mLanguageName( languageName ),
	mFiles( files ),
	mPatterns( patterns ),
	mSymbols( symbols ),
	mComment( comment ),
	mHeaders( headers ) {}

const std::vector<std::string>& SyntaxDefinition::getFiles() const {
	return mFiles;
}

std::string SyntaxDefinition::getFileExtension() const {
	if ( !mFiles.empty() ) {
		std::string ext( mFiles[0] );
		String::replaceAll( ext, "%", "" );
		String::replaceAll( ext, "$", "" );
		String::replaceAll( ext, "?", "" );
		return ext;
	}
	return "";
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
		return it->second;
	return "";
}

SyntaxDefinition& SyntaxDefinition::addFileType( const std::string& fileType ) {
	mFiles.push_back( fileType );
	return *this;
}

SyntaxDefinition& SyntaxDefinition::addPattern( const SyntaxPattern& pattern ) {
	mPatterns.push_back( pattern );
	return *this;
}

SyntaxDefinition& SyntaxDefinition::addPatternToFront( const SyntaxPattern& pattern ) {
	auto patterns = mPatterns;
	mPatterns.clear();
	mPatterns.push_back( pattern );
	for ( auto pa : patterns )
		mPatterns.push_back( pa );
	return *this;
}

SyntaxDefinition& SyntaxDefinition::addSymbol( const std::string& symbolName,
											   const std::string& typeName ) {
	mSymbols[symbolName] = typeName;
	return *this;
}

SyntaxDefinition& SyntaxDefinition::addSymbols( const std::vector<std::string>& symbolNames,
												const std::string& typeName ) {
	for ( auto& symbol : symbolNames ) {
		addSymbol( symbol, typeName );
	}
	return *this;
}

SyntaxDefinition& SyntaxDefinition::setComment( const std::string& comment ) {
	mComment = comment;
	return *this;
}

const std::vector<std::string>& SyntaxDefinition::getHeaders() const {
	return mHeaders;
}

SyntaxDefinition& SyntaxDefinition::setHeaders( const std::vector<std::string>& headers ) {
	mHeaders = headers;
	return *this;
}

const std::string& SyntaxDefinition::getLanguageName() const {
	return mLanguageName;
}

}}} // namespace EE::UI::Doc
