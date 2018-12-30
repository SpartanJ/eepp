#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetProperty::StyleSheetProperty()
{}

StyleSheetProperty::StyleSheetProperty( const std::string& name, const std::string& value ) :
	mName( String::toLower( String::trim( name ) ) ),
	mValue( String::trim( value ) )
{}

const std::string &StyleSheetProperty::getName() const {
	return mName;
}

const std::string &StyleSheetProperty::getValue() const {
	return mValue;
}

const Uint32 &StyleSheetProperty::getSpecificity() const {
	return mSpecificity;
}

}}}
