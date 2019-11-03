#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetProperty::StyleSheetProperty()
{}

StyleSheetProperty::StyleSheetProperty( const std::string& name, const std::string& value ) :
	mName( String::toLower( String::trim( name ) ) ),
	mValue( String::trim( value ) ),
	mSpecificity( 0 )
{}

StyleSheetProperty::StyleSheetProperty( const std::string & name, const std::string& value, const Uint32 & specificity, const bool& isVolatile ) :
	mName( String::toLower( String::trim( name ) ) ),
	mValue( String::trim( value ) ),
	mSpecificity( specificity ),
	mVolatile( isVolatile )
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

void StyleSheetProperty::setSpecificity( const Uint32 & specificity ) {
	mSpecificity = specificity;
}

bool StyleSheetProperty::isEmpty() const {
	return mName.empty();
}

void StyleSheetProperty::setName( const std::string& name ) {
	mName = name;
}

void StyleSheetProperty::setValue( const std::string& value ) {
	mValue = value;
}

const bool &StyleSheetProperty::isVolatile() const {
	return mVolatile;
}

void StyleSheetProperty::setVolatile( const bool & isVolatile ) {
	mVolatile = isVolatile;
}

}}}
