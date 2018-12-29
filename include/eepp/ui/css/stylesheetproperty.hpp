#ifndef EE_UI_CSS_STYLESHEETPROPERTY_HPP
#define EE_UI_CSS_STYLESHEETPROPERTY_HPP

#include <string>
#include <map>
#include <eepp/config.hpp>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetProperty {
	public:
		StyleSheetProperty();

		explicit StyleSheetProperty( const std::string& name, const std::string& value );

		std::string name;
		std::string value;
		Uint32 specificity;
};

typedef std::map<std::string, StyleSheetProperty> StyleSheetProperties;
typedef std::map<std::string, StyleSheetProperty> PropertiesDictionary;

}}}

#endif
