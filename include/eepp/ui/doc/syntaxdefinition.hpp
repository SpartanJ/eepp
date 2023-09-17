#ifndef EE_UI_DOC_DEFINITION_HPP
#define EE_UI_DOC_DEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace EE { namespace UI { namespace Doc {

struct EE_API SyntaxPattern {
	std::vector<std::string> patterns;
	std::vector<std::string> types;
	std::string syntax{ "" };

	SyntaxPattern( std::vector<std::string> _patterns, std::string _type,
				   std::string _syntax = "" ) :
		patterns( _patterns ), types( { _type } ), syntax( _syntax ) {}

	SyntaxPattern( std::vector<std::string> _patterns, std::vector<std::string> _types,
				   std::string _syntax = "" ) :
		patterns( _patterns ), types( _types ), syntax( _syntax ) {}
};

class EE_API SyntaxDefinition {
  public:
	SyntaxDefinition();

	SyntaxDefinition( const std::string& languageName, const std::vector<std::string>& files,
					  const std::vector<SyntaxPattern>& patterns,
					  const std::unordered_map<std::string, std::string>& symbols =
						  std::unordered_map<std::string, std::string>(),
					  const std::string& comment = "", const std::vector<std::string> headers = {},
					  const std::string& lspName = "" );

	const std::string& getLanguageName() const;

	std::string getLanguageNameForFileSystem() const;

	const String::HashType& getLanguageId() const;

	const std::vector<std::string>& getFiles() const;

	std::string getFileExtension() const;

	const std::vector<SyntaxPattern>& getPatterns() const;

	const std::string& getComment() const;

	const std::unordered_map<std::string, std::string>& getSymbols() const;

	std::string getSymbol( const std::string& symbol ) const;

	/** Accepts lua patterns and file extensions. */
	SyntaxDefinition& addFileType( const std::string& fileType );

	SyntaxDefinition& addPattern( const SyntaxPattern& pattern );

	SyntaxDefinition& setPatterns( const std::vector<SyntaxPattern>& patterns );

	SyntaxDefinition& addPatternToFront( const SyntaxPattern& pattern );

	SyntaxDefinition& addPatternsToFront( const std::vector<SyntaxPattern>& patterns );

	SyntaxDefinition& addSymbol( const std::string& symbolName, const std::string& typeName );

	SyntaxDefinition& addSymbols( const std::vector<std::string>& symbolNames,
								  const std::string& typeName );

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

	std::vector<SyntaxPattern> getPatternsOfType( const std::string& type ) const;

	SyntaxDefinition& setFileTypes( const std::vector<std::string>& types );

	bool hasExtensionPriority() const;

	void setExtensionPriority( bool hasExtensionPriority );

  protected:
	std::string mLanguageName;
	String::HashType mLanguageId;
	std::vector<std::string> mFiles;
	std::vector<SyntaxPattern> mPatterns;
	std::unordered_map<std::string, std::string> mSymbols;
	std::string mComment;
	std::vector<std::string> mHeaders;
	std::string mLSPName;
	bool mAutoCloseXMLTags{ false };
	bool mVisible{ true };
	bool mHasExtensionPriority{ false };
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLE_HPP
