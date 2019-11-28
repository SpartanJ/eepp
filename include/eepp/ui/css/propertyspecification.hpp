#ifndef EE_UI_CSS_PROPERTYSPECIFICATION_HPP
#define EE_UI_CSS_PROPERTYSPECIFICATION_HPP

#include <eepp/ui/css/propertydefinition.hpp>
#include <memory>

namespace EE { namespace UI { namespace CSS {

class EE_API PropertySpecification {
  public:
	PropertyDefinition& registerProperty( const std::string& propertyVame,
										  const std::string& defaultValue, bool inherited );

	const PropertyDefinition* getProperty( const Uint32& id ) const;

	const PropertyDefinition* getProperty( const std::string& name ) const;

  protected:
	std::vector<std::unique_ptr<PropertyDefinition>> mProperties;
};

}}} // namespace EE::UI::CSS

#endif
