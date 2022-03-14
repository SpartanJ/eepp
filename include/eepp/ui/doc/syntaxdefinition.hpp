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
	std::string type;
	std::string syntax{ "" };
};

class EE_API SyntaxDefinition {
  public:
	SyntaxDefinition();

	SyntaxDefinition( const std::string& languageName, const std::vector<std::string>& files,
					  const std::vector<SyntaxPattern>& patterns,
					  const std::unordered_map<std::string, std::string>& symbols =
						  std::unordered_map<std::string, std::string>(),
					  const std::string& comment = "",
					  const std::vector<std::string> headers = {} );

	const std::string& getLanguageName() const;

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

	SyntaxDefinition& addPatternToFront( const SyntaxPattern& pattern );

	SyntaxDefinition& addSymbol( const std::string& symbolName, const std::string& typeName );

	SyntaxDefinition& addSymbols( const std::vector<std::string>& symbolNames,
								  const std::string& typeName );

	/** Sets the comment string used for auto-comment functionality. */
	SyntaxDefinition& setComment( const std::string& comment );

	const std::vector<std::string>& getHeaders() const;

	SyntaxDefinition& setHeaders( const std::vector<std::string>& headers );

	void clearPatterns();

	void clearSymbols();

  protected:
	std::string mLanguageName;
	String::HashType mLanguageId;
	std::vector<std::string> mFiles;
	std::vector<SyntaxPattern> mPatterns;
	std::unordered_map<std::string, std::string> mSymbols;
	std::string mComment;
	std::vector<std::string> mHeaders;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLE_HPP
