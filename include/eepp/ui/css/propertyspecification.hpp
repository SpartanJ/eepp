#ifndef EE_UI_CSS_PROPERTYSPECIFICATION_HPP
#define EE_UI_CSS_PROPERTYSPECIFICATION_HPP

#include <eepp/system/singleton.hpp>
#include <eepp/ui/css/idnamemap.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/propertyidset.hpp>
#include <eepp/ui/css/shorthanddefinition.hpp>
#include <memory>
#include <vector>

namespace EE { namespace UI { namespace CSS {

class EE_API PropertySpecification {
	SINGLETON_DECLARE_HEADERS( PropertySpecification )
  public:
	~PropertySpecification();

	PropertyDefinition& registerProperty( PropertyId id, const std::string& propertyName,
										  const std::string& defaultValue, bool inherited );

	PropertyDefinition* registerProperty( const std::string& propertyName,
										  const std::string& defaultValue, bool inherited );

	const PropertyDefinition* getProperty( const PropertyId& id ) const;

	const PropertyDefinition* getProperty( const char* name ) const;

	const PropertyDefinition* getProperty( std::string_view name ) const;

	const PropertyDefinition* getProperty( const std::string& name ) const;

	ShorthandDefinition& registerShorthand( ShorthandId id, const std::string& name,
											const std::vector<std::string>& properties,
											const std::string& shorthandParserName );

	ShorthandDefinition* registerShorthand( const std::string& name,
											const std::vector<std::string>& properties,
											const std::string& shorthandParserName );

	const ShorthandDefinition* getShorthand( const ShorthandId& id ) const;

	const ShorthandDefinition* getShorthand( const std::string& name ) const;

	bool isShorthand( const std::string& name ) const;

	const PropertyIdSet& getInheritableProperties() const;

	void finalizeBuiltins();

  protected:
	friend class PropertyDefinition;
	friend class ShorthandDefinition;

	IdNameMap<PropertyId, 512> mPropertyIds;
	IdNameMap<ShorthandId, 255> mShorthandIds;
	std::vector<std::shared_ptr<PropertyDefinition>> mPropertiesById;
	std::vector<std::shared_ptr<ShorthandDefinition>> mShorthandsById;
	PropertyIdSet mInheritableProperties;

	const PropertyDefinition* addPropertyAlias( const std::string& alias,
												const PropertyDefinition* propDef );

	const ShorthandDefinition* addShorthandAlias( const std::string& alias,
												  const ShorthandDefinition* shorthandDef );
};

}}} // namespace EE::UI::CSS

#endif
