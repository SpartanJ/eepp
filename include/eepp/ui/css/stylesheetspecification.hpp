#ifndef EE_UI_CSS_STYLESHEETSPECIFICATION_HPP
#define EE_UI_CSS_STYLESHEETSPECIFICATION_HPP

#include <eepp/core.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/ui/css/drawableimageparser.hpp>
#include <eepp/ui/css/shorthanddefinition.hpp>
#include <functional>

namespace EE { namespace UI {
class UIWidget;
}} // namespace EE::UI

namespace EE { namespace UI { namespace CSS {

class PropertySpecification;

typedef std::function<bool( const UIWidget* node, int a, int b, const FunctionString& data )>
	StyleSheetNodeSelector;

struct StructuralSelector {
	StructuralSelector( StyleSheetNodeSelector selector, int a = 0, int b = 0,
						const FunctionString& data = FunctionString::parse( "" ) ) :
		selector( selector ), a( a ), b( b ), data( data ) {}
	StyleSheetNodeSelector selector;
	int a;
	int b;
	FunctionString data;
};

class EE_API StyleSheetSpecification {
	SINGLETON_DECLARE_HEADERS( StyleSheetSpecification )
  public:
	StyleSheetSpecification();

	~StyleSheetSpecification();

	PropertyDefinition& registerProperty( const std::string& propertyVame,
										  const std::string& defaultValue, bool inherited = false );

	const PropertyDefinition* getProperty( const Uint32& id ) const;

	const PropertyDefinition* getProperty( const std::string& name ) const;

	ShorthandDefinition& registerShorthand( const std::string& name,
											const std::vector<std::string>& properties,
											const std::string& shorthandFuncName );

	const ShorthandDefinition* getShorthand( const Uint32& id ) const;

	const ShorthandDefinition* getShorthand( const std::string& name ) const;

	bool isShorthand( const Uint32& id ) const;

	bool isShorthand( const std::string& name ) const;

	void registerNodeSelector( const std::string& name, StyleSheetNodeSelector nodeSelector );

	StructuralSelector getStructuralSelector( const std::string& name );

	void registerShorthandParser( const std::string& name,
								  ShorthandParserFunc shorthandParserFunc );

	ShorthandParserFunc getShorthandParser( const std::string& name );

	DrawableImageParser& getDrawableImageParser();

  protected:
	PropertySpecification* mPropertySpecification;
	DrawableImageParser mDrawableImageParser;
	UnorderedMap<std::string, ShorthandParserFunc> mShorthandParsers;
	UnorderedMap<std::string, StyleSheetNodeSelector> mNodeSelectors;

	void registerDefaultShorthandParsers();

	void registerDefaultProperties();

	void registerDefaultNodeSelectors();
};

}}} // namespace EE::UI::CSS

#endif
