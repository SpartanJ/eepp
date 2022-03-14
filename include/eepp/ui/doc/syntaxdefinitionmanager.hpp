#ifndef EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
#define EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP

#include <eepp/config.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <vector>

namespace EE { namespace UI { namespace Doc {

class EE_API SyntaxDefinitionManager {
	SINGLETON_DECLARE_HEADERS( SyntaxDefinitionManager )
  public:
	SyntaxDefinition& add( SyntaxDefinition&& syntaxStyle );

	const SyntaxDefinition& getPlainStyle() const;

	const SyntaxDefinition& getStyleByExtension( const std::string& filePath ) const;

	const SyntaxDefinition& getStyleByHeader( const std::string& header ) const;

	const SyntaxDefinition& find( const std::string& filePath, const std::string& header );

	SyntaxDefinition& getStyleByExtensionRef( const std::string& filePath );

	const SyntaxDefinition& getStyleByLanguageName( const std::string& name ) const;

	const SyntaxDefinition& getStyleByLanguageId( const String::HashType& id) const;

	SyntaxDefinition& getStyleByLanguageNameRef( const std::string& name );

	std::vector<std::string> getLanguageNames() const;

	std::vector<std::string> getExtensionsPatternsSupported() const;

  protected:
	SyntaxDefinitionManager();

	std::vector<SyntaxDefinition> mStyles;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
