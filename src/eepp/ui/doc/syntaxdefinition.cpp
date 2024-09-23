#include <eepp/core/memorymanager.hpp>
#include <eepp/core/string.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>

namespace EE { namespace UI { namespace Doc {

UnorderedMap<SyntaxStyleType, std::string> SyntaxPattern::SyntaxStyleTypeCache = {};

template <typename SyntaxStyleType> void updateCache( const SyntaxPattern& ptrn ) {
	if constexpr ( std::is_same_v<SyntaxStyleType, std::string> ) {
		return;
	} else {
		for ( size_t i = 0; i < ptrn.typesNames.size(); i++ ) {
			if ( SyntaxStyleTypes::needsToBeCached( ptrn.types[i] ) ) {
				auto it = SyntaxPattern::SyntaxStyleTypeCache.find( ptrn.types[i] );
				if ( it == SyntaxPattern::SyntaxStyleTypeCache.end() )
					SyntaxPattern::SyntaxStyleTypeCache[ptrn.types[i]] = ptrn.typesNames[i];
			}
		}
	}
}

SyntaxDefinition::SyntaxDefinition() {}

SyntaxDefinition::SyntaxDefinition( const std::string& languageName,
									std::vector<std::string>&& files,
									std::vector<SyntaxPattern>&& patterns,
									UnorderedMap<std::string, std::string>&& symbols,
									const std::string& comment, std::vector<std::string>&& headers,
									const std::string& lspName ) :
	mLanguageName( languageName ),
	mLanguageId( String::hash( String::toLower( languageName ) ) ),
	mFiles( std::move( files ) ),
	mPatterns( std::move( patterns ) ),
	mSymbolNames( std::move( symbols ) ),
	mComment( comment ),
	mHeaders( std::move( headers ) ),
	mLSPName( lspName.empty() ? String::toLower( mLanguageName ) : lspName ) {
	mSymbols.reserve( mSymbolNames.size() );
	for ( const auto& symbol : mSymbolNames )
		mSymbols.insert( { symbol.first, toSyntaxStyleType( symbol.second ) } );
}

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

std::vector<SyntaxPattern>
SyntaxDefinition::getPatternsOfType( const SyntaxStyleType& type ) const {
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

bool SyntaxDefinition::hasExtensionPriority() const {
	return mHasExtensionPriority;
}

SyntaxDefinition& SyntaxDefinition::setExtensionPriority( bool hasExtensionPriority ) {
	mHasExtensionPriority = hasExtensionPriority;
	return *this;
}

UnorderedMap<std::string, std::string> SyntaxDefinition::getSymbolNames() const {
	return mSymbolNames;
}

bool SyntaxDefinition::isCaseInsensitive() const {
	return mCaseInsensitive;
}

SyntaxDefinition& SyntaxDefinition::setCaseInsensitive( bool caseInsensitive ) {
	mCaseInsensitive = caseInsensitive;
	return *this;
}

FoldRangeType SyntaxDefinition::getFoldRangeType() const {
	return mFoldRangeType;
}

SyntaxDefinition& SyntaxDefinition::setFoldRangeType( FoldRangeType foldRangeType ) {
	mFoldRangeType = foldRangeType;
	return *this;
}

std::vector<std::pair<Int64, Int64>> SyntaxDefinition::getFoldBraces() const {
	return mFoldBraces;
}

SyntaxDefinition&
SyntaxDefinition::setFoldBraces( const std::vector<std::pair<Int64, Int64>>& foldBraces ) {
	mFoldBraces = foldBraces;
	return *this;
}

const std::vector<SyntaxPattern>& SyntaxDefinition::getPatterns() const {
	return mPatterns;
}

const std::string& SyntaxDefinition::getComment() const {
	return mComment;
}

const UnorderedMap<std::string, SyntaxStyleType>& SyntaxDefinition::getSymbols() const {
	return mSymbols;
}

SyntaxStyleType SyntaxDefinition::getSymbol( const std::string& symbol ) const {
	auto it = mSymbols.find( mCaseInsensitive ? String::toLower( symbol ) : symbol );
	if ( it != mSymbols.end() )
		return it->second;
	return SyntaxStyleEmpty();
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
	mSymbols[symbolName] = toSyntaxStyleType( typeName );
	mSymbolNames[symbolName] = typeName;
	return *this;
}

SyntaxDefinition& SyntaxDefinition::addSymbols( const std::vector<std::string>& symbolNames,
												const std::string& typeName ) {
	for ( auto& symbol : symbolNames )
		addSymbol( symbol, typeName );
	return *this;
}

SyntaxDefinition&
SyntaxDefinition::setSymbols( const UnorderedMap<std::string, SyntaxStyleType>& symbols ) {
	mSymbols = symbols;
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

std::string SyntaxDefinition::getLanguageNameForFileSystem() const {
	std::string lang( mLanguageName );
	String::replaceAll( lang, " ", "" );
	String::replaceAll( lang, ".", "" );
	String::replaceAll( lang, "!", "" );
	String::replaceAll( lang, "[", "" );
	String::replaceAll( lang, "]", "" );
	String::replaceAll( lang, "+", "p" );
	String::replaceAll( lang, "#", "sharp" );
	String::toLowerInPlace( lang );
	return lang;
}

const String::HashType& SyntaxDefinition::getLanguageId() const {
	return mLanguageId;
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns, const std::string& _type,
							  const std::string& _syntax, bool isRegEx ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( std::vector<std::string>{ _type } ) ),
	typesNames( { _type } ),
	syntax( _syntax ),
	isRegEx( isRegEx ) {
	updateCache<SyntaxStyleType>( *this );
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns,
							  std::vector<std::string>&& _types, const std::string& _syntax,
							  bool isRegEx ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( _types ) ),
	typesNames( std::move( _types ) ),
	syntax( _syntax ),
	isRegEx( isRegEx ) {
	updateCache<SyntaxStyleType>( *this );
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns, const std::string& _type,
							  DynamicSyntax&& _syntax, bool isRegEx ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( std::vector<std::string>{ _type } ) ),
	typesNames( { _type } ),
	dynSyntax( std::move( _syntax ) ),
	isRegEx( isRegEx ) {
	updateCache<SyntaxStyleType>( *this );
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns,
							  std::vector<std::string>&& _types, DynamicSyntax&& _syntax,
							  bool isRegEx ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( _types ) ),
	typesNames( std::move( _types ) ),
	dynSyntax( std::move( _syntax ) ),
	isRegEx( isRegEx ) {
	updateCache<SyntaxStyleType>( *this );
}

}}} // namespace EE::UI::Doc
