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
									const std::vector<std::string> headers,
									const std::string& lspName ) :
	mLanguageName( languageName ),
	mLanguageId( String::hash( String::toLower( languageName ) ) ),
	mFiles( files ),
	mPatterns( patterns ),
	mSymbols( symbols ),
	mComment( comment ),
	mHeaders( headers ),
	mLSPName( lspName.empty() ? String::toLower( mLanguageName ) : lspName ) {}

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

std::vector<SyntaxPattern> SyntaxDefinition::getPatternsOfType( const std::string& type ) const {
	std::vector<SyntaxPattern> patterns;
	for ( const auto& pattern : mPatterns ) {
		if ( pattern.types.size() == 1 && pattern.types[0] == type )
			patterns.emplace_back( pattern );
	}
	return patterns;
}

SyntaxDefinition& SyntaxDefinition::setFileTypes( const std::vector<std::string>& types ) {
	mFiles = types;
	return *this;
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

SyntaxDefinition& SyntaxDefinition::setPatterns( const std::vector<SyntaxPattern>& patterns ) {
	mPatterns = patterns;
	return *this;
}

SyntaxDefinition& SyntaxDefinition::addPatternToFront( const SyntaxPattern& pattern ) {
	mPatterns.insert( mPatterns.begin(), pattern );
	return *this;
}

SyntaxDefinition&
SyntaxDefinition::addPatternsToFront( const std::vector<SyntaxPattern>& patterns ) {
	mPatterns.insert( mPatterns.begin(), patterns.begin(), patterns.end() );
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

void SyntaxDefinition::clearPatterns() {
	mPatterns.clear();
}

void SyntaxDefinition::clearSymbols() {
	mSymbols.clear();
}

const std::string& SyntaxDefinition::getLSPName() const {
	return mLSPName;
}

SyntaxDefinition& SyntaxDefinition::setVisible( bool visible ) {
	mVisible = visible;
	return *this;
}

bool SyntaxDefinition::isVisible() const {
	return mVisible;
}

bool SyntaxDefinition::getAutoCloseXMLTags() const {
	return mAutoCloseXMLTags;
}

SyntaxDefinition& SyntaxDefinition::setAutoCloseXMLTags( bool autoCloseXMLTags ) {
	mAutoCloseXMLTags = autoCloseXMLTags;
	return *this;
}

SyntaxDefinition& SyntaxDefinition::setLanguageName( const std::string& languageName ) {
	mLanguageName = languageName;
	mLSPName = String::toLower( languageName );
	mLanguageId = String::hash( mLSPName );
	return *this;
}

SyntaxDefinition& SyntaxDefinition::setLSPName( const std::string& lSPName ) {
	mLSPName = lSPName;
	return *this;
}

const std::string& SyntaxDefinition::getLanguageName() const {
	return mLanguageName;
}

const String::HashType& SyntaxDefinition::getLanguageId() const {
	return mLanguageId;
}

}}} // namespace EE::UI::Doc
