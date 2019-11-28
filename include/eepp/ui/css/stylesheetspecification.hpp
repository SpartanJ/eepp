#ifndef EE_UI_CSS_STYLESHEETSPECIFICATION_HPP
#define EE_UI_CSS_STYLESHEETSPECIFICATION_HPP

#include <eepp/core.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/ui/css/propertyspecification.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheetSpecification {
	SINGLETON_DECLARE_HEADERS( StyleSheetSpecification )
  public:
	StyleSheetSpecification();

	~StyleSheetSpecification();

	PropertyDefinition& registerProperty( const std::string& propertyVame,
										  const std::string& defaultValue, bool inherited );

	const PropertyDefinition* getProperty( const Uint32& id ) const;

	const PropertyDefinition* getProperty( const std::string& name ) const;

  protected:
	PropertySpecification mPropertySpecification;

	void registerDefaultProperties();
};

}}} // namespace EE::UI::CSS

#endif
