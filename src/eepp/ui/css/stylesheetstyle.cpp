#include <eepp/ui/css/stylesheetstyle.hpp>
#include <iostream>

namespace EE { namespace UI { namespace CSS {

StyleSheetStyle::StyleSheetStyle()
{}

StyleSheetStyle::StyleSheetStyle( const std::string& selector, const StyleSheetProperties& properties ) :
	mSelector( selector ),
	mProperties( properties )
{
	for ( auto it = mProperties.begin(); it != mProperties.end(); ++it )
		it->second.setSpecificity( mSelector.getSpecificity() );
}

void StyleSheetStyle::print() {
	std::cout << mSelector.getName() << " {" << std::endl;

	for ( StyleSheetProperties::iterator it = mProperties.begin(); it != mProperties.end(); ++it ) {
		StyleSheetProperty& prop = it->second;

		std::cout << "\t" << prop.getName() << ": " << prop.getValue() << ";" << std::endl;
	}

	std::cout << "}" << std::endl;
}

const StyleSheetSelector& StyleSheetStyle::getSelector() const {
	return mSelector;
}

const StyleSheetProperties& StyleSheetStyle::getProperties() const {
	return mProperties;
}

StyleSheetProperty StyleSheetStyle::getPropertyByName( const std::string& name ) const {
	auto it = mProperties.find( name );

	if ( it != mProperties.end() )
		return it->second;

	return StyleSheetProperty();
}

void StyleSheetStyle::setProperty( const StyleSheetProperty & property ) {
	mProperties[ property.getName() ] = property;
}

void StyleSheetStyle::clearProperties() {
	mProperties.clear();
}

}}}
