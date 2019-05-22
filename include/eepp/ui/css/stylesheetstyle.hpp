#ifndef EE_UI_CSS_STYLESHEETSTYLE_HPP
#define EE_UI_CSS_STYLESHEETSTYLE_HPP

#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetStyle {
	public:
		StyleSheetStyle();

		explicit StyleSheetStyle( const std::string& selector, const StyleSheetProperties& properties );

		std::string build();

		const StyleSheetSelector& getSelector() const;

		const StyleSheetProperties& getProperties() const;

		StyleSheetProperty getPropertyByName( const std::string& name ) const;

		void setProperty( const StyleSheetProperty& property );

		void clearProperties();
	protected:
		StyleSheetSelector mSelector;
		StyleSheetProperties mProperties;
};

typedef std::map<std::string, StyleSheetStyle> StyleSheetStyleList;
typedef std::vector<StyleSheetStyle> StyleSheetStyleVector;

}}}

#endif
