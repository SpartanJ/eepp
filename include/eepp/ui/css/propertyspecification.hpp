#ifndef EE_UI_CSS_PROPERTYSPECIFICATION_HPP
#define EE_UI_CSS_PROPERTYSPECIFICATION_HPP

#include <eepp/system/singleton.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/shorthanddefinition.hpp>
#include <memory>
#include <unordered_map>

namespace EE { namespace UI { namespace CSS {

class EE_API PropertySpecification {
	SINGLETON_DECLARE_HEADERS( PropertySpecification )
  public:
	~PropertySpecification();

	PropertyDefinition& registerProperty( const std::string& propertyVame,
										  const std::string& defaultValue, bool inherited );

	const PropertyDefinition* getProperty( const Uint32& id ) const;

	const PropertyDefinition* getProperty( const std::string& name ) const;

	ShorthandDefinition& registerShorthand( const std::string& name,
											const std::vector<std::string>& properties,
											const std::string& shorthandParserName );

	const ShorthandDefinition* getShorthand( const String::HashType& id ) const;

	const ShorthandDefinition* getShorthand( const std::string& name ) const;

	bool isShorthand( const std::string& name ) const;

	bool isShorthand( const Uint32& id ) const;

  protected:
	friend class PropertyDefinition;
	std::unordered_map<Uint32, std::shared_ptr<PropertyDefinition>> mProperties;
	std::unordered_map<Uint32, std::shared_ptr<ShorthandDefinition>> mShorthands;

	const PropertyDefinition* addPropertyAlias( Uint32 aliasId, const PropertyDefinition* propDef );
};

}}} // namespace EE::UI::CSS

#endif
