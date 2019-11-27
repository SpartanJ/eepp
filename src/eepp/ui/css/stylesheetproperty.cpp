#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselectorrule.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetProperty::StyleSheetProperty() :
	mSpecificity( 0 ),
	mVolatile( false ),
	mImportant( false )
{}

StyleSheetProperty::StyleSheetProperty( const std::string& name, const std::string& value ) :
	mName( String::toLower( String::trim( name ) ) ),
	mNameHash( String::hash( mName ) ),
	mValue( String::trim( value ) ),
	mSpecificity( 0 ),
	mVolatile( false ),
	mImportant( false )
{
	checkImportant();
}

StyleSheetProperty::StyleSheetProperty( const std::string & name, const std::string& value, const Uint32 & specificity, const bool& isVolatile ) :
	mName( String::toLower( String::trim( name ) ) ),
	mNameHash( String::hash( mName ) ),
	mValue( String::trim( value ) ),
	mSpecificity( specificity ),
	mVolatile( isVolatile ),
	mImportant( false )
{
	checkImportant();
}

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
	// Don't allow set specificity if is an important property,
	// force the specificity in this case.
	if ( !mImportant ) {
		mSpecificity = specificity;
	}
}

bool StyleSheetProperty::isEmpty() const {
	return mName.empty();
}

void StyleSheetProperty::setName( const std::string& name ) {
	mName = name;
	mNameHash = String::hash( mName );
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

bool StyleSheetProperty::operator==( const StyleSheetProperty& property ) {
	return mNameHash == property.mNameHash && mValue == property.mValue;
}

const Uint32& StyleSheetProperty::getNameHash() const {
	return mNameHash;
}

void StyleSheetProperty::checkImportant() {
	if ( String::endsWith( mValue, "!important" ) ) {
		mImportant = true;
		mSpecificity = StyleSheetSelectorRule::SpecificityImportant;
		mValue = String::trim( mValue.substr( 0, mValue.size() - 10/*!important*/ ) );
	}
}

}}}
