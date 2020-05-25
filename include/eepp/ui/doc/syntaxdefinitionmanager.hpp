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

	SyntaxDefinition& getStyleByExtensionRef( const std::string& filePath );

  protected:
	SyntaxDefinitionManager();

	std::vector<SyntaxDefinition> mStyles;
	SyntaxDefinition mEmptyDefinition;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXSTYLEMANAGER_HPP
