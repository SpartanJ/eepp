#ifndef EE_UI_DOC_DEFINITION_HPP
#define EE_UI_DOC_DEFINITION_HPP

#include <eepp/config.hpp>
#include <unordered_map>
#include <vector>

namespace EE { namespace UI { namespace Doc {

struct EE_API SyntaxPattern {
	std::vector<std::string> patterns;
	std::string type;
};

class EE_API SyntaxDefinition {
  public:
	SyntaxDefinition();

	SyntaxDefinition( const std::vector<std::string>& files,
					  const std::vector<SyntaxPattern>& patterns,
					  const std::unordered_map<std::string, std::string>& symbols =
						  std::unordered_map<std::string, std::string>(),
					  const std::string& comment = "" );

	const std::vector<std::string>& getFiles() const;

	const std::vector<SyntaxPattern>& getPatterns() const;

	const std::string& getComment() const;

	const std::unordered_map<std::string, std::string>& getSymbols() const;

	std::string getSymbol( const std::string& symbol ) const;

  protected:
	std::vector<std::string> mFiles;
	std::vector<SyntaxPattern> mPatterns;
	std::unordered_map<std::string, std::string> mSymbols;
	std::string mComment;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLE_HPP
