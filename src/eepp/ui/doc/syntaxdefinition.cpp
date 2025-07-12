#include <eepp/core/memorymanager.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/parsermatcher.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

using namespace std::literals;

namespace EE { namespace UI { namespace Doc {

static constexpr String::HashType toLowerAndHash( const char* str, Int64 len ) {
	String::HashType hash = 5381;
	while ( --len >= 0 )
		hash = ( ( hash << 5 ) + hash ) + std::tolower( *str++ );
	return hash;
}

static constexpr String::HashType toLowerAndHash( std::string_view str ) {
	return toLowerAndHash( str.data(), str.size() );
}

SyntaxDefMap<SyntaxStyleType, std::string> SyntaxPattern::SyntaxStyleTypeCache = {};

static void liftContentPatternsRecursive( SyntaxDefinition& def, SyntaxPattern& pattern,
										  std::string_view namePrefixSeed,
										  Uint64& uniqueIdCounter ) {
	for ( Uint64 i = 0; i < pattern.contentPatterns.size(); ++i ) {
		// Pass a new prefix seed for children to ensure unique names
		std::string childPrefixSeed = String::format( "%s_cp%zu", namePrefixSeed, i );
		liftContentPatternsRecursive( def, pattern.contentPatterns[i], childPrefixSeed,
									  uniqueIdCounter );
	}

	if ( !pattern.contentPatterns.empty() ) {
		// Generate a unique repository name for this pattern's content scope
		std::string contentRepoName =
			String::format( "$CONTENT_%s_%s_uid%zu", def.getLanguageNameForFileSystem(),
							namePrefixSeed, uniqueIdCounter++ );
		pattern.contentScopeRepoHash = String::hash( contentRepoName );
		def.addRepository( std::move( contentRepoName ), std::move( pattern.contentPatterns ) );
	}
}

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

		for ( size_t i = 0; i < ptrn.endTypesNames.size(); i++ ) {
			if ( SyntaxStyleTypes::needsToBeCached( ptrn.endTypes[i] ) ) {
				auto it = SyntaxPattern::SyntaxStyleTypeCache.find( ptrn.endTypes[i] );
				if ( it == SyntaxPattern::SyntaxStyleTypeCache.end() )
					SyntaxPattern::SyntaxStyleTypeCache[ptrn.endTypes[i]] = ptrn.endTypesNames[i];
			}
		}
	}
}

static void updatePatternRefs( const SyntaxDefinition& def, SyntaxPattern& ptrn ) {
	ptrn.def = &def;
	if ( ptrn.syntax == "$self" || ptrn.syntax == "$base" )
		ptrn.syntax = def.getLanguageName();
}

static void updatePatternState( SyntaxDefinition& def, SyntaxPattern& ptrn ) {
	if ( ptrn.checkIsRangedMatch() )
		ptrn.flags |= SyntaxPattern::IsRangedMatch;

	if ( ptrn.checkIsIncludePattern() ) {
		ptrn.flags |= SyntaxPattern::IsInclude;

		if ( ptrn.checkIsRepositoryInclude() ) {
			ptrn.flags |= SyntaxPattern::IsRepositoryInclude;
			ptrn.repositoryIdx = def.getRepositoryIndex( ptrn.getRepositoryName() );
		} else if ( ptrn.checkIsRootSelfInclude() ) {
			ptrn.flags |= SyntaxPattern::IsRootSelfInclude;
		} else if ( ptrn.checkIsSourceInclude() && ptrn.patterns[1].size() > 7 ) {
			ptrn.flags |= SyntaxPattern::IsSourceInclude;

			if ( def.getRepositoryName( String::hash( ptrn.patterns[1] ) ).empty() ) {
				const auto& lang = SyntaxDefinitionManager::instance()->getByLanguageName(
					std::string_view{ ptrn.patterns[1] }.substr( 7 /* "source." */ ) );

				auto syntaxRepoName = ptrn.patterns[1];
				auto patterns = lang.getPatterns();
				for ( auto& iptrn : patterns ) {
					if ( iptrn.isRepositoryInclude() ) {
						iptrn.flags = SyntaxPattern::IsInclude | SyntaxPattern::IsSourceInclude;
						iptrn.patterns[1] = String::format( "%s.%s", syntaxRepoName,
															iptrn.patterns[1].substr( 1 ) );
					}
				}
				def.addRepository( std::move( syntaxRepoName ), { std::move( patterns ) } );
				const auto& repos = lang.getRepositories();
				std::vector<std::pair<std::string, std::vector<SyntaxPattern>>> nrepos;
				nrepos.reserve( repos.size() );
				for ( const auto& repo : repos ) {
					const auto& repoName = lang.getRepositoryName( repo.first );
					auto newRepoName = String::format( "%s.%s", ptrn.patterns[1], repoName );
					auto npatterns = repo.second.patterns;
					for ( auto& iptrn : npatterns ) {
						if ( iptrn.isRepositoryInclude() ) {
							iptrn.flags = SyntaxPattern::IsInclude | SyntaxPattern::IsSourceInclude;
							iptrn.patterns[1] = String::format( "%s.%s", ptrn.patterns[1],
																iptrn.patterns[1].substr( 1 ) );
						}
					}
					nrepos.emplace_back( std::move( newRepoName ), npatterns );
				}
			}
		} else {
			Log::warning( "updatePatternState unknown include directive: %s", ptrn.patterns[1] );
			ptrn.flags &= ~SyntaxPattern::IsInclude;
		}
	} else {
		ptrn.flags |= SyntaxPattern::IsPure;
	}

	updatePatternRefs( def, ptrn );
}

static void updatePatternsState( SyntaxDefinition& def, std::vector<SyntaxPattern>& ptrns ) {
	for ( auto& ptrn : ptrns ) {
		updatePatternState( def, ptrn );

		for ( auto& subPattern : ptrn.contentPatterns )
			updatePatternState( def, subPattern );
	}
}

static void updateRepoIndexState( SyntaxDefinition& def, std::vector<SyntaxPattern>& ptrns ) {
	for ( auto& ptrn : ptrns )
		if ( ptrn.isRepositoryInclude() )
			ptrn.repositoryIdx = def.getRepositoryIndex( ptrn.getRepositoryName() );
}

SyntaxDefinition::SyntaxDefinition() {}

SyntaxDefinition::SyntaxDefinition(
	const std::string& languageName, std::vector<std::string>&& files,
	std::vector<SyntaxPattern>&& patterns, SyntaxDefMap<std::string, std::string>&& symbols,
	const std::string& comment, std::vector<std::string>&& headers, const std::string& lspName,
	std::vector<std::pair<std::string, std::vector<SyntaxPattern>>>&& repositories ) :
	mLanguageName( languageName ),
	mFiles( std::move( files ) ),
	mPatterns( std::move( patterns ) ),
	mSymbolNames( std::move( symbols ) ),
	mComment( comment ),
	mHeaders( std::move( headers ) ),
	mLSPName( lspName.empty() ? String::toLower( mLanguageName ) : lspName ) {
	mSymbolsHashes.reserve( mSymbolNames.size() );
	if ( !mPatterns.empty() ) {
		if ( !ParserMatcherManager::instance()->registeredBaseParsers() &&
			 std::any_of( mPatterns.begin(), mPatterns.end(), []( const SyntaxPattern& pattern ) {
				 return pattern.matchType == SyntaxPatternMatchType::Parser;
			 } ) )
			ParserMatcherManager::instance()->registerBaseParsers();
		updatePatternsState( *this, mPatterns );
		mPatterns.emplace_back( SyntaxPattern{ { "%s+" }, "normal" } );
		mPatterns.emplace_back( SyntaxPattern{ { "%w+%f[%s]" }, "normal" } );
	}
	for ( const auto& symbol : mSymbolNames ) {
		mSymbolsHashes.insert(
			{ String::hash( symbol.first ), toSyntaxStyleType( symbol.second ) } );
	}
	addRepositories( std::move( repositories ) );
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

SyntaxDefMap<std::string, std::string> SyntaxDefinition::getSymbolNames() const {
	return mSymbolNames;
}

bool SyntaxDefinition::isCaseInsensitive() const {
	return mCaseInsensitive;
}

SyntaxDefinition& SyntaxDefinition::setCaseInsensitive( bool caseInsensitive ) {
	if ( mCaseInsensitive != caseInsensitive ) {
		mCaseInsensitive = caseInsensitive;
		mSymbolsHashes.clear();
		for ( const auto& symbol : mSymbolNames ) {
			mSymbolsHashes.insert(
				{ mCaseInsensitive ? toLowerAndHash( symbol.first ) : String::hash( symbol.first ),
				  toSyntaxStyleType( symbol.second ) } );
		}
	}
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

const SyntaxDefMap<String::HashType, SyntaxStyleType>& SyntaxDefinition::getSymbols() const {
	return mSymbolsHashes;
}

SyntaxStyleType SyntaxDefinition::getSymbol( std::string_view symbol ) const {
	auto it =
		mSymbolsHashes.find( mCaseInsensitive ? toLowerAndHash( symbol ) : String::hash( symbol ) );
	if ( it != mSymbolsHashes.end() )
		return it->second;
	return SyntaxStyleEmpty();
}

SyntaxDefinition& SyntaxDefinition::addFileType( const std::string& fileType ) {
	mFiles.push_back( fileType );
	return *this;
}

SyntaxDefinition& SyntaxDefinition::addPattern( const SyntaxPattern& pattern ) {
	mPatterns.push_back( pattern );
	updatePatternState( *this, mPatterns[mPatterns.size() - 1] );
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
	mSymbolsHashes[mCaseInsensitive ? toLowerAndHash( symbolName ) : String::hash( symbolName )] =
		toSyntaxStyleType( typeName );
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
SyntaxDefinition::setSymbols( const SyntaxDefMap<String::HashType, SyntaxStyleType>& symbols,
							  const SyntaxDefMap<std::string, std::string>& symbolNames ) {
	mSymbolsHashes = symbols;
	mSymbolNames = symbolNames;
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

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns, const std::string& _type,
							  const std::string& _syntax, SyntaxPatternMatchType matchType ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( std::vector<std::string>{ _type } ) ),
	typesNames( { _type } ),
	syntax( _syntax ),
	matchType( matchType ) {
	updateCache<SyntaxStyleType>( *this );
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns,
							  std::vector<std::string>&& _types, const std::string& _syntax,
							  SyntaxPatternMatchType matchType ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( _types ) ),
	typesNames( std::move( _types ) ),
	syntax( _syntax ),
	matchType( matchType ) {
	updateCache<SyntaxStyleType>( *this );
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns,
							  std::vector<std::string>&& _types,
							  std::vector<std::string>&& _endTypes, const std::string& _syntax,
							  SyntaxPatternMatchType matchType,
							  std::vector<SyntaxPattern>&& _subPatterns ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( _types ) ),
	endTypes( toSyntaxStyleTypeV( _endTypes ) ),
	typesNames( std::move( _types ) ),
	endTypesNames( std::move( _endTypes ) ),
	syntax( _syntax ),
	matchType( matchType ),
	contentPatterns( std::move( _subPatterns ) ) {
	eeASSERT( patterns.size() < std::numeric_limits<Uint8>::max() - 1 );
	updateCache<SyntaxStyleType>( *this );
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns,
							  std::vector<std::string>&& _types,
							  std::vector<std::string>&& _endTypes, const std::string& _syntax,
							  SyntaxPatternMatchType matchType ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( _types ) ),
	endTypes( toSyntaxStyleTypeV( _endTypes ) ),
	typesNames( std::move( _types ) ),
	endTypesNames( std::move( _endTypes ) ),
	syntax( _syntax ),
	matchType( matchType ) {
	eeASSERT( patterns.size() < std::numeric_limits<Uint8>::max() - 1 );
	updateCache<SyntaxStyleType>( *this );
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns, const std::string& _type,
							  DynamicSyntax&& _syntax, SyntaxPatternMatchType matchType ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( std::vector<std::string>{ _type } ) ),
	typesNames( { _type } ),
	dynSyntax( std::move( _syntax ) ),
	matchType( matchType ) {
	eeASSERT( patterns.size() < std::numeric_limits<Uint8>::max() - 1 );
	updateCache<SyntaxStyleType>( *this );
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns,
							  std::vector<std::string>&& _types, DynamicSyntax&& _syntax,
							  SyntaxPatternMatchType matchType ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( _types ) ),
	typesNames( std::move( _types ) ),
	dynSyntax( std::move( _syntax ) ),
	matchType( matchType ) {
	eeASSERT( patterns.size() < std::numeric_limits<Uint8>::max() - 1 );
	updateCache<SyntaxStyleType>( *this );
}

SyntaxPattern::SyntaxPattern( std::vector<std::string>&& _patterns,
							  std::vector<std::string>&& _types,
							  std::vector<std::string>&& _endTypes, DynamicSyntax&& _syntax,
							  SyntaxPatternMatchType matchType ) :
	patterns( std::move( _patterns ) ),
	types( toSyntaxStyleTypeV( _types ) ),
	endTypes( toSyntaxStyleTypeV( _endTypes ) ),
	typesNames( std::move( _types ) ),
	endTypesNames( std::move( _endTypes ) ),
	dynSyntax( std::move( _syntax ) ),
	matchType( matchType ) {
	eeASSERT( patterns.size() < std::numeric_limits<Uint8>::max() - 1 );
	updateCache<SyntaxStyleType>( *this );
}

SyntaxDefinition& SyntaxDefinition::addRepository( std::string&& name,
												   SyntaxRepository&& repository ) {

	eeASSERT( repository.patterns.size() < std::numeric_limits<SyntaxSyateHolderType>::max() - 1 );
	auto hash = String::hash( name );
	mRepositoryIndex[hash] = ++mRepositoryIndexCounter;
	mRepositoryIndexInvert[mRepositoryIndexCounter] = hash;
	for ( auto& ptrn : repository.patterns )
		ptrn.repositoryIdx = mRepositoryIndexCounter;
	updatePatternsState( *this, repository.patterns );
	mRepository[hash] = std::move( repository );
	updateRepoIndexState( *this, mPatterns );
	mRepositoryNames[hash] = std::move( name );
	return *this;
}

SyntaxDefinition& SyntaxDefinition::addRepositories(
	std::vector<std::pair<std::string, std::vector<SyntaxPattern>>>&& repositories ) {
	if ( !repositories.empty() ) {
		for ( auto& ptrn : repositories ) {
			SyntaxRepository repo( std::move( ptrn.second ) );
			addRepository( std::move( ptrn.first ), std::move( repo ) );
		}
		compile();
	}
	return *this;
}

const SyntaxRepository& SyntaxDefinition::getRepository( String::HashType hash ) const {
	static SyntaxRepository EMPTY = SyntaxRepository();
	auto found = mRepository.find( hash );
	return found != mRepository.end() ? found->second : EMPTY;
}

const SyntaxRepository& SyntaxDefinition::getRepository( std::string_view name ) const {
	return getRepository( String::hash( name ) );
}

Uint32 SyntaxDefinition::getRepositoryIndex( String::HashType hash ) const {
	auto it = mRepositoryIndex.find( hash );
	return it != mRepositoryIndex.end() ? it->second : 0;
}

Uint32 SyntaxDefinition::getRepositoryIndex( std::string_view name ) const {
	return getRepositoryIndex( String::hash( name ) );
}

String::HashType SyntaxDefinition::getRepositoryHash( Uint32 index ) const {
	auto it = mRepositoryIndexInvert.find( index );
	return it != mRepositoryIndexInvert.end() ? it->second : 0;
}

SyntaxDefinition& SyntaxDefinition::addAlternativeName( const std::string& name ) {
	mLanguageAlternativeNames.emplace_back( name );
	return *this;
}

const std::vector<std::string>& SyntaxDefinition::getAlternativeNames() const {
	return mLanguageAlternativeNames;
}

const SyntaxPattern* SyntaxDefinition::getPatternFromState( const SyntaxStateType& state ) const {
	if ( state.repositoryIdx == 0 && state.state > 0 &&
		 state.state - 1 < static_cast<int>( mPatterns.size() ) ) {
		return &mPatterns[state.state - 1];
	} else if ( state.repositoryIdx != 0 ) {
		auto hash = getRepositoryHash( state.repositoryIdx );
		if ( hash ) {
			const auto& repo = mRepository.find( hash );
			if ( repo != mRepository.end() && state.state > 0 &&
				 state.state - 1 < static_cast<int>( repo->second.patterns.size() ) ) {
				return &repo->second.patterns[state.state - 1];
			}
		}
		eeASSERT( false );
	}
	return nullptr;
}

const SyntaxDefMap<String::HashType, SyntaxRepository>& SyntaxDefinition::getRepositories() const {
	return mRepository;
}

const SyntaxDefMap<String::HashType, std::string>& SyntaxDefinition::getRepositoriesNames() const {
	return mRepositoryNames;
}

std::string SyntaxDefinition::getRepositoryName( String::HashType hash ) const {
	auto it = mRepositoryNames.find( hash );
	return it != mRepositoryNames.end() ? it->second : "";
}

void SyntaxDefinition::compile() {
	Uint64 uniqueIdCounter = 0;
	for ( SyntaxPattern& p : mPatterns )
		liftContentPatternsRecursive( *this, p, "root", uniqueIdCounter );

	if ( mRepository.empty() )
		return;

	std::vector<std::pair<String::HashType, SyntaxRepository*>> curRepos;
	curRepos.reserve( mRepository.size() );
	for ( auto& repoPair : mRepository )
		curRepos.emplace_back( repoPair.first, &repoPair.second );

	for ( auto& repoPair : curRepos ) {
		const auto& name = mRepositoryNames[repoPair.first];
		for ( SyntaxPattern& p : repoPair.second->patterns ) {
			liftContentPatternsRecursive( *this, p, name, uniqueIdCounter );
		}
	}

	for ( auto& repoPair : curRepos )
		updateRepoIndexState( *this, repoPair.second->patterns );
}

}}} // namespace EE::UI::Doc
