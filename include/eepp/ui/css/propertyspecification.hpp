#ifndef EE_UI_CSS_PROPERTYSPECIFICATION_HPP
#define EE_UI_CSS_PROPERTYSPECIFICATION_HPP

#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/shorthanddefinition.hpp>
#include <memory>

namespace EE { namespace UI { namespace CSS {

class EE_API PropertySpecification {
  public:
	~PropertySpecification();

	PropertyDefinition& registerProperty( const std::string& propertyVame,
										  const std::string& defaultValue, bool inherited );

	const PropertyDefinition* getProperty( const Uint32& id ) const;

	const PropertyDefinition* getProperty( const std::string& name ) const;

	ShorthandDefinition& registerShorthand( const std::string& name,
											const std::vector<std::string>& properties,
											const std::string& shorthandParserName );

	const ShorthandDefinition* getShorthand( const Uint32& id ) const;

	const ShorthandDefinition* getShorthand( const std::string& name ) const;

	bool isShorthand( const std::string& name ) const;

	bool isShorthand( const Uint32& id ) const;

  protected:
	std::vector<PropertyDefinition*> mProperties;
	std::vector<ShorthandDefinition*> mShorthands;
};

}}} // namespace EE::UI::CSS

#endif
