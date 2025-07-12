#ifndef EE_UI_DOC_DEFINITION_HPP
#define EE_UI_DOC_DEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/ui/doc/foldrangetype.hpp>
#include <eepp/ui/doc/syntaxcolorscheme.hpp>
#include <eepp/ui/doc/syntaxtokenizer.hpp>

#include <string>
#include <type_traits>
#include <vector>

namespace EE { namespace UI { namespace Doc {

template <typename T> static auto toSyntaxStyleTypeV( const std::vector<T>& s ) noexcept {
	if constexpr ( std::is_same_v<SyntaxStyleType, std::string> &&
				   std::is_same_v<T, std::string> ) {
		return std::vector<T>( s );
	} else if constexpr ( std::is_same_v<SyntaxStyleType, String::HashType> &&
						  std::is_same_v<T, std::string> ) {
		std::vector<SyntaxStyleType> v;
		v.reserve( s.size() );
		for ( const auto& sv : s )
			v.push_back( String::hash( sv ) );
		return v;
	} else
		return std::vector<SyntaxStyleType>{};
}

enum class SyntaxPatternMatchType { LuaPattern, RegEx, Parser };

class SyntaxDefinition;

template <typename Key, typename Value> using SyntaxDefMap = std::unordered_map<Key, Value>;

struct EE_API SyntaxPattern {
	enum Flags {
		IsPure = 1 << 0,
		IsInclude = 1 << 1,
		IsRepositoryInclude = 1 << 2,
		IsRootSelfInclude = 1 << 3,
		IsRangedMatch = 1 << 5,
		IsSourceInclude = 1 << 6,
	};

	static SyntaxDefMap<SyntaxStyleType, std::string> SyntaxStyleTypeCache;

	using DynamicSyntax =
		std::function<std::string( const SyntaxPattern&, const std::string_view& )>;

	std::vector<std::string> patterns;
	std::vector<SyntaxStyleType> types;
	std::vector<SyntaxStyleType> endTypes;
	std::vector<std::string> typesNames;
	std::vector<std::string> endTypesNames;
	std::string syntax{ "" };
	DynamicSyntax dynSyntax;
	SyntaxPatternMatchType matchType{ SyntaxPatternMatchType::LuaPattern };
	const SyntaxDefinition* def{ nullptr };
	Uint16 flags{ 0 };
	Uint16 repositoryIdx{ 0 };
	std::vector<SyntaxPattern> contentPatterns;
	String::HashType contentScopeRepoHash{
		0 }; // Hash of the repository containing this rule's content patterns

	SyntaxPattern( std::vector<std::string>&& _patterns, const std::string& _type,
				   const std::string& _syntax = "",
				   SyntaxPatternMatchType matchType = SyntaxPatternMatchType::LuaPattern );

	SyntaxPattern( std::vector<std::string>&& _patterns, std::vector<std::string>&& _types,
				   const std::string& _syntax = "",
				   SyntaxPatternMatchType matchType = SyntaxPatternMatchType::LuaPattern );

	SyntaxPattern( std::vector<std::string>&& _patterns, std::vector<std::string>&& _types,
				   std::vector<std::string>&& _endTypes, const std::string& _syntax = "",
				   SyntaxPatternMatchType matchType = SyntaxPatternMatchType::LuaPattern );

	SyntaxPattern( std::vector<std::string>&& _patterns, std::vector<std::string>&& _types,
				   std::vector<std::string>&& _endTypes, const std::string& _syntax,
				   SyntaxPatternMatchType matchType, std::vector<SyntaxPattern>&& _subPatterns );

	SyntaxPattern( std::vector<std::string>&& _patterns, const std::string& _type,
				   DynamicSyntax&& _syntax,
				   SyntaxPatternMatchType matchType = SyntaxPatternMatchType::LuaPattern );

	SyntaxPattern( std::vector<std::string>&& _patterns, std::vector<std::string>&& _types,
				   DynamicSyntax&& _syntax,
				   SyntaxPatternMatchType matchType = SyntaxPatternMatchType::LuaPattern );

	SyntaxPattern( std::vector<std::string>&& _patterns, std::vector<std::string>&& _types,
				   std::vector<std::string>&& _endTypes, DynamicSyntax&& _syntax,
				   SyntaxPatternMatchType matchType = SyntaxPatternMatchType::LuaPattern );

	inline bool hasSyntax() const { return !syntax.empty() || dynSyntax; }

	inline bool hasSyntaxOrContentScope() const { return hasSyntax() || hasContentScope(); }

	inline bool isPure() const { return flags & Flags::IsPure; }

	inline bool isInclude() const { return flags & Flags::IsInclude; }

	inline bool isRepositoryInclude() const { return flags & Flags::IsRepositoryInclude; }

	inline bool isRootSelfInclude() const { return flags & Flags::IsRootSelfInclude; }

	inline bool isSourceInclude() const { return flags & Flags::IsSourceInclude; }

	inline bool isRangedMatch() const { return flags & Flags::IsRangedMatch; }

	inline bool isSimpleRangedMatch() const {
		return isRangedMatch() && !hasContentScope() && !hasSyntax();
	}

	std::string_view getRepositoryName() const {
		eeASSERT( isRepositoryInclude() || isSourceInclude() );
		return isSourceInclude() ? std::string_view{ patterns[1] }
								 : std::string_view{ patterns[1] }.substr( 1 );
	}

	inline bool checkIsIncludePattern() const {
		return patterns.size() == 2 && patterns[0] == "include" && !patterns[1].empty() &&
			   ( patterns[1][0] == '#' || patterns[1][0] == '$' ||
				 String::startsWith( patterns[1], "source." ) );
	}

	inline bool checkIsRangedMatch() const {
		return !checkIsIncludePattern() && patterns.size() >= 2;
	}

	inline bool checkIsRootSelfInclude() const {
		return checkIsIncludePattern() && ( patterns[1] == "$self" || patterns[1] == "$base" );
	}

	inline bool checkIsSourceInclude() const {
		return checkIsIncludePattern() && String::startsWith( patterns[1], "source." );
	}

	inline bool checkIsRepositoryInclude() const {
		return checkIsIncludePattern() && patterns[1][0] == '#';
	}

	inline bool hasContentScope() const { return contentScopeRepoHash != 0; }
};

struct EE_API SyntaxRepository {
	std::vector<SyntaxPattern> patterns;

	SyntaxRepository() {}

	SyntaxRepository( std::vector<SyntaxPattern>&& patterns ) : patterns( std::move( patterns ) ) {}
};

struct EE_API SyntaxPreDefinition {
	std::string name;
	std::function<SyntaxDefinition&()> load;
	std::vector<std::string> files;
	std::vector<std::string> headers;
	std::string lspName;
	std::vector<std::string> alternativeNames;
	bool extensionPriority{ false };

	SyntaxPreDefinition( const std::string& name, std::function<SyntaxDefinition&()> load,
						 std::vector<std::string>&& files, std::vector<std::string>&& headers = {},
						 const std::string& lspName = "",
						 std::vector<std::string>&& alternativeNames = {},
						 bool extensionPriority = false ) :
		name( name ),
		load( load ),
		files( std::move( files ) ),
		headers( std::move( headers ) ),
		lspName( lspName.empty() ? String::toLower( name ) : lspName ),
		alternativeNames( std::move( alternativeNames ) ),
		extensionPriority( extensionPriority ) {}

	const std::string& getLanguageName() const { return name; }

	const std::vector<std::string>& getAlternativeNames() const { return alternativeNames; }

	const std::string& getLSPName() const { return lspName; }

	const std::vector<std::string>& getFiles() const { return files; }

	bool hasExtensionPriority() const { return extensionPriority; }

	const std::vector<std::string>& getHeaders() const { return headers; }
};

class EE_API SyntaxDefinition {
  public:
	SyntaxDefinition();

	SyntaxDefinition(
		const std::string& languageName, std::vector<std::string>&& files,
		std::vector<SyntaxPattern>&& patterns,
		SyntaxDefMap<std::string, std::string>&& symbols = {}, const std::string& comment = "",
		std::vector<std::string>&& headers = {}, const std::string& lspName = "",
		std::vector<std::pair<std::string, std::vector<SyntaxPattern>>>&& repositories = {} );

	const std::string& getLanguageName() const;

	std::string getLanguageNameForFileSystem() const;

	const std::vector<std::string>& getFiles() const;

	std::string getFileExtension() const;

	const std::vector<SyntaxPattern>& getPatterns() const;

	const std::string& getComment() const;

	const SyntaxDefMap<String::HashType, SyntaxStyleType>& getSymbols() const;

	SyntaxStyleType getSymbol( std::string_view symbol ) const;

	/** Accepts lua patterns and file extensions. */
	SyntaxDefinition& addFileType( const std::string& fileType );

	SyntaxDefinition& addPattern( const SyntaxPattern& pattern );

	SyntaxDefinition& setPatterns( const std::vector<SyntaxPattern>& patterns );

	SyntaxDefinition& addPatternToFront( const SyntaxPattern& pattern );

	SyntaxDefinition& addPatternsToFront( const std::vector<SyntaxPattern>& patterns );

	SyntaxDefinition& addSymbol( const std::string& symbolName, const std::string& typeName );

	SyntaxDefinition& addSymbols( const std::vector<std::string>& symbolNames,
								  const std::string& typeName );

	SyntaxDefinition& setSymbols( const SyntaxDefMap<String::HashType, SyntaxStyleType>& symbols,
								  const SyntaxDefMap<std::string, std::string>& symbolNames );

	/** Sets the comment string used for auto-comment functionality. */
	SyntaxDefinition& setComment( const std::string& comment );

	const std::vector<std::string>& getHeaders() const;

	SyntaxDefinition& setHeaders( const std::vector<std::string>& headers );

	void clearPatterns();

	const std::string& getLSPName() const;

	SyntaxDefinition& setVisible( bool visible );

	bool isVisible() const;

	bool getAutoCloseXMLTags() const;

	SyntaxDefinition& setAutoCloseXMLTags( bool autoCloseXMLTags );

	SyntaxDefinition& setLanguageName( const std::string& languageName );

	SyntaxDefinition& setLSPName( const std::string& lSPName );

	std::vector<SyntaxPattern> getPatternsOfType( const SyntaxStyleType& type ) const;

	SyntaxDefinition& setFileTypes( const std::vector<std::string>& types );

	bool hasExtensionPriority() const;

	SyntaxDefinition& setExtensionPriority( bool hasExtensionPriority );

	SyntaxDefMap<std::string, std::string> getSymbolNames() const;

	const Uint16& getLanguageIndex() const { return mLanguageIndex; }

	bool isCaseInsensitive() const;

	SyntaxDefinition& setCaseInsensitive( bool caseInsensitive );

	FoldRangeType getFoldRangeType() const;

	SyntaxDefinition& setFoldRangeType( FoldRangeType foldRangeType );

	std::vector<std::pair<Int64, Int64>> getFoldBraces() const;

	SyntaxDefinition& setFoldBraces( const std::vector<std::pair<Int64, Int64>>& foldBraces );

	SyntaxDefinition& addRepository( std::string&& name, SyntaxRepository&& patterns );

	SyntaxDefinition& addRepositories(
		std::vector<std::pair<std::string, std::vector<SyntaxPattern>>>&& repositories );

	const SyntaxRepository& getRepository( String::HashType hash ) const;

	const SyntaxRepository& getRepository( std::string_view name ) const;

	Uint32 getRepositoryIndex( String::HashType hash ) const;

	Uint32 getRepositoryIndex( std::string_view name ) const;

	String::HashType getRepositoryHash( Uint32 index ) const;

	std::string getRepositoryName( String::HashType hash ) const;

	const SyntaxDefMap<String::HashType, SyntaxRepository>& getRepositories() const;

	const SyntaxDefMap<String::HashType, std::string>& getRepositoriesNames() const;

	SyntaxDefinition& addAlternativeName( const std::string& name );

	const std::vector<std::string>& getAlternativeNames() const;

	const SyntaxPattern* getPatternFromState( const SyntaxStateType& state ) const;

	void compile();

  protected:
	friend class SyntaxDefinitionManager;

	std::string mLanguageName;
	std::vector<std::string> mFiles;
	std::vector<SyntaxPattern> mPatterns;
	SyntaxDefMap<String::HashType, SyntaxStyleType> mSymbolsHashes;
	SyntaxDefMap<std::string, std::string> mSymbolNames;
	std::string mComment;
	std::vector<std::string> mHeaders;
	std::string mLSPName;
	Uint16 mLanguageIndex{ 0 };
	FoldRangeType mFoldRangeType{ FoldRangeType::Undefined };
	std::vector<std::pair<Int64, Int64>> mFoldBraces;
	bool mAutoCloseXMLTags{ false };
	bool mVisible{ true };
	bool mHasExtensionPriority{ false };
	bool mCaseInsensitive{ false };
	SyntaxDefMap<String::HashType, SyntaxRepository> mRepository;
	SyntaxDefMap<String::HashType, Uint32> mRepositoryIndex;
	SyntaxDefMap<String::HashType, std::string> mRepositoryNames;
	SyntaxDefMap<Uint32, String::HashType> mRepositoryIndexInvert;
	std::vector<std::string> mLanguageAlternativeNames;
	Uint32 mRepositoryIndexCounter{ 0 };
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLE_HPP
