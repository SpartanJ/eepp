#include <eepp/ui/css/stylesheetstyle.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetStyle::StyleSheetStyle()
{}

StyleSheetStyle::StyleSheetStyle( const std::string& selector, const StyleSheetProperties& properties ) :
	mSelector( selector ),
	mProperties( properties )
{
	for ( auto& it : mProperties ) {
		it.second.setSpecificity( mSelector.getSpecificity() );
		it.second.setVolatile( !mSelector.isCacheable() );
	}
}

std::string StyleSheetStyle::build() {
	std::string css;

	css += mSelector.getName() + " {";


	for ( auto& it : mProperties ) {
		StyleSheetProperty& prop = it.second;

		css += "\t" + prop.getName() + ": " + prop.getValue() + ";\n";
	}

	css += "}\n";

	return css;
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
