#ifndef EE_UI_DOC_DEFINITION_HPP
#define EE_UI_DOC_DEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/ui/doc/foldrangetype.hpp>
#include <eepp/ui/doc/syntaxcolorscheme.hpp>
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

struct EE_API SyntaxPattern {
	static UnorderedMap<SyntaxStyleType, std::string> SyntaxStyleTypeCache;

	using DynamicSyntax =
		std::function<std::string( const SyntaxPattern&, const std::string_view& )>;

	std::vector<std::string> patterns;
	std::vector<SyntaxStyleType> types;
	std::vector<std::string> typesNames;
	std::string syntax{ "" };
	DynamicSyntax dynSyntax;
	bool isRegEx{ false };

	SyntaxPattern( std::vector<std::string>&& _patterns, const std::string& _type,
				   const std::string& _syntax = "", bool isRegEx = false );

	SyntaxPattern( std::vector<std::string>&& _patterns, std::vector<std::string>&& _types,
				   const std::string& _syntax = "", bool isRegEx = false );

	SyntaxPattern( std::vector<std::string>&& _patterns, const std::string& _type,
				   DynamicSyntax&& _syntax, bool isRegEx = false );

	SyntaxPattern( std::vector<std::string>&& _patterns, std::vector<std::string>&& _types,
				   DynamicSyntax&& _syntax, bool isRegEx = false );

	bool hasSyntax() const { return !syntax.empty() || dynSyntax; }
};

class EE_API SyntaxDefinition {
  public:
	SyntaxDefinition();

	SyntaxDefinition( const std::string& languageName, std::vector<std::string>&& files,
					  std::vector<SyntaxPattern>&& patterns,
					  UnorderedMap<std::string, std::string>&& symbols = {},
					  const std::string& comment = "", std::vector<std::string>&& headers = {},
					  const std::string& lspName = "" );

	const std::string& getLanguageName() const;

	std::string getLanguageNameForFileSystem() const;

	const String::HashType& getLanguageId() const;

	const std::vector<std::string>& getFiles() const;

	std::string getFileExtension() const;

	const std::vector<SyntaxPattern>& getPatterns() const;

	const std::string& getComment() const;

	const UnorderedMap<std::string, SyntaxStyleType>& getSymbols() const;

	SyntaxStyleType getSymbol( const std::string& symbol ) const;

	/** Accepts lua patterns and file extensions. */
	SyntaxDefinition& addFileType( const std::string& fileType );

	SyntaxDefinition& addPattern( const SyntaxPattern& pattern );

	SyntaxDefinition& setPatterns( const std::vector<SyntaxPattern>& patterns );

	SyntaxDefinition& addPatternToFront( const SyntaxPattern& pattern );

	SyntaxDefinition& addPatternsToFront( const std::vector<SyntaxPattern>& patterns );

	SyntaxDefinition& addSymbol( const std::string& symbolName, const std::string& typeName );

	SyntaxDefinition& addSymbols( const std::vector<std::string>& symbolNames,
								  const std::string& typeName );

	SyntaxDefinition& setSymbols( const UnorderedMap<std::string, SyntaxStyleType>& symbols );

	/** Sets the comment string used for auto-comment functionality. */
	SyntaxDefinition& setComment( const std::string& comment );

	const std::vector<std::string>& getHeaders() const;

	SyntaxDefinition& setHeaders( const std::vector<std::string>& headers );

	void clearPatterns();

	void clearSymbols();

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

	UnorderedMap<std::string, std::string> getSymbolNames() const;

	const Uint16& getLanguageIndex() const { return mLanguageIndex; }

	bool isCaseInsensitive() const;

	SyntaxDefinition& setCaseInsensitive( bool caseInsensitive );

	FoldRangeType getFoldRangeType() const;

	SyntaxDefinition& setFoldRangeType( FoldRangeType foldRangeType );

	std::vector<std::pair<Int64, Int64>> getFoldBraces() const;

	SyntaxDefinition& setFoldBraces( const std::vector<std::pair<Int64, Int64>>& foldBraces );

  protected:
	friend class SyntaxDefinitionManager;

	std::string mLanguageName;
	String::HashType mLanguageId;
	std::vector<std::string> mFiles;
	std::vector<SyntaxPattern> mPatterns;
	UnorderedMap<std::string, SyntaxStyleType> mSymbols;
	UnorderedMap<std::string, std::string> mSymbolNames;
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
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLE_HPP
