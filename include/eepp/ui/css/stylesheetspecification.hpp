#ifndef EE_UI_CSS_STYLESHEETSPECIFICATION_HPP
#define EE_UI_CSS_STYLESHEETSPECIFICATION_HPP

#include <eepp/core.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/ui/css/propertyspecification.hpp>

namespace EE { namespace UI {
class UIWidget;
}} // namespace EE::UI

namespace EE { namespace UI { namespace CSS {

typedef std::function<bool( const UIWidget* node, int a, int b )> StyleSheetNodeSelector;

struct StructuralSelector {
	StructuralSelector( StyleSheetNodeSelector selector, int a, int b ) :
		selector( selector ), a( a ), b( b ) {}
	StyleSheetNodeSelector selector;
	int a;
	int b;
};

class StyleSheetSpecification {
	SINGLETON_DECLARE_HEADERS( StyleSheetSpecification )
  public:
	StyleSheetSpecification();

	~StyleSheetSpecification();

	PropertyDefinition& registerProperty( const std::string& propertyVame,
										  const std::string& defaultValue, bool inherited );

	const PropertyDefinition* getProperty( const Uint32& id ) const;

	const PropertyDefinition* getProperty( const std::string& name ) const;

	ShorthandDefinition& registerShorthand( const std::string& name,
											const std::vector<std::string>& properties,
											const ShorthandType& shorthandType );

	const ShorthandDefinition* getShorthand( const Uint32& id ) const;

	const ShorthandDefinition* getShorthand( const std::string& name ) const;

	bool isShorthand( const Uint32& id ) const;

	bool isShorthand( const std::string& name ) const;

	void registerNodeSelector( const std::string& name, StyleSheetNodeSelector nodeSelector );

	StructuralSelector getStructuralSelector( const std::string& name );

  protected:
	PropertySpecification mPropertySpecification;
	std::map<std::string, StyleSheetNodeSelector> mNodeSelectors;

	void registerDefaultProperties();

	void registerDefaultNodeSelectors();
};

}}} // namespace EE::UI::CSS

#endif
