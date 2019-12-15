#ifndef EE_UI_CSS_STYLESHEETSTYLE_HPP
#define EE_UI_CSS_STYLESHEETSTYLE_HPP

#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetvariable.hpp>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetStyle {
  public:
	StyleSheetStyle();

	explicit StyleSheetStyle( const std::string& selector, const StyleSheetProperties& properties,
							  const StyleSheetVariables& variables,
							  MediaQueryList::ptr mediaQueryList );

	std::string build();

	const StyleSheetSelector& getSelector() const;

	const StyleSheetProperties& getProperties() const;

	StyleSheetProperties& getPropertiesRef();

	const StyleSheetVariables& getVariables() const;

	StyleSheetProperty getPropertyByName( const std::string& name ) const;

	void setProperty( const StyleSheetProperty& property );

	void clearProperties();

	StyleSheetVariable getVariableByName( const std::string& name ) const;

	void setVariable( const StyleSheetVariable& variable );

	bool isMediaValid() const;

	const MediaQueryList::ptr& getMediaQueryList() const;

  protected:
	StyleSheetSelector mSelector;
	StyleSheetProperties mProperties;
	StyleSheetVariables mVariables;
	MediaQueryList::ptr mMediaQueryList;
};

typedef std::map<std::string, StyleSheetStyle> StyleSheetStyleList;
typedef std::vector<StyleSheetStyle> StyleSheetStyleVector;

}}} // namespace EE::UI::CSS

#endif
