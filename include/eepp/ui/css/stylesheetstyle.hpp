#ifndef EE_UI_CSS_STYLESHEETSTYLE_HPP
#define EE_UI_CSS_STYLESHEETSTYLE_HPP

#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetvariable.hpp>

namespace EE { namespace UI { namespace CSS {

enum class AtRuleType : Uint32 { None, FontFace, GlyphIcon };

class EE_API StyleSheetStyle {
  public:
	StyleSheetStyle();

	explicit StyleSheetStyle( const std::string& selector, const StyleSheetProperties& properties,
							  const StyleSheetVariables& variables,
							  MediaQueryList::ptr mediaQueryList );

	std::string build( bool emmitMediaQueryStart = true, bool emmitMediaQueryEnd = true );

	const StyleSheetSelector& getSelector() const;

	const StyleSheetProperties& getProperties() const;

	StyleSheetProperties& getPropertiesRef();

	const StyleSheetVariables& getVariables() const;

	const StyleSheetProperty* getPropertyById( const PropertyId& id ) const;

	const StyleSheetProperty* getPropertyByDefinition( const PropertyDefinition* def ) const;

	StyleSheetProperty* getPropertyById( const Uint32& id );

	void setProperty( const StyleSheetProperty& property );

	void clearProperties();

	bool hasProperties() const;

	bool hasProperty( PropertyId id ) const;

	bool hasProperty( const std::string& name ) const;

	bool hasVariables() const;

	bool hasVariable( const std::string& name ) const;

	StyleSheetVariable getVariableByName( const std::string& name ) const;

	void setVariable( const StyleSheetVariable& variable );

	bool isMediaValid() const;

	const MediaQueryList::ptr& getMediaQueryList() const;

	bool isAtRule() const;

	const AtRuleType& getAtRuleType() const;

	const Uint32& getMarker() const;

	void setMarker( const Uint32& marker );

	bool updatePropertyValue( const std::string& name, const std::string& value );

  protected:
	Uint32 mMarker{ 0 };
	StyleSheetSelector mSelector;
	StyleSheetProperties mProperties;
	StyleSheetVariables mVariables;
	MediaQueryList::ptr mMediaQueryList;
	AtRuleType mAtRuleType;

	AtRuleType checkAtRule();
};

typedef std::vector<StyleSheetStyle*> StyleSheetStyleVector;

}}} // namespace EE::UI::CSS

#endif
