#include <eepp/ui/css/stylesheetstyle.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetStyle::StyleSheetStyle() {}

StyleSheetStyle::StyleSheetStyle( const std::string& selector,
								  const StyleSheetProperties& properties,
								  const StyleSheetVariables& variables,
								  MediaQueryList::ptr mediaQueryList ) :
	mSelector( selector ),
	mProperties( properties ),
	mVariables( variables ),
	mMediaQueryList( mediaQueryList ),
	mAtRuleType( checkAtRule() ) {
	for ( auto& it : mProperties ) {
		it.second.setSpecificity( mSelector.getSpecificity() );
		it.second.setVolatile( !mSelector.isCacheable() );
	}
	for ( auto& it : mVariables ) {
		it.second.setSpecificity( mSelector.getSpecificity() );
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

StyleSheetProperties& StyleSheetStyle::getPropertiesRef() {
	return mProperties;
}

const StyleSheetVariables& StyleSheetStyle::getVariables() const {
	return mVariables;
}

StyleSheetProperty StyleSheetStyle::getPropertyById( const PropertyId& id ) const {
	for ( auto& prop : mProperties ) {
		if ( NULL != prop.second.getPropertyDefinition() &&
			 prop.second.getPropertyDefinition()->getPropertyId() == id ) {
			return prop.second;
		}
	}

	return StyleSheetProperty();
}

StyleSheetProperty StyleSheetStyle::getPropertyByDefinition( const PropertyDefinition* def ) const {
	for ( auto& prop : mProperties ) {
		if ( NULL != prop.second.getPropertyDefinition() &&
			 prop.second.getPropertyDefinition() == def ) {
			return prop.second;
		}
	}

	return StyleSheetProperty();
}

StyleSheetProperty StyleSheetStyle::getPropertyByName( const std::string& name ) const {
	auto it = mProperties.find( name );

	if ( it != mProperties.end() )
		return it->second;

	return StyleSheetProperty();
}

void StyleSheetStyle::setProperty( const StyleSheetProperty& property ) {
	mProperties[property.getName()] = property;
}

void StyleSheetStyle::clearProperties() {
	mProperties.clear();
}

StyleSheetVariable StyleSheetStyle::getVariableByName( const std::string& name ) const {
	auto it = mVariables.find( name );

	if ( it != mVariables.end() )
		return it->second;

	return StyleSheetVariable();
}

void StyleSheetStyle::setVariable( const StyleSheetVariable& variable ) {
	mVariables[variable.getName()] = variable;
}

bool StyleSheetStyle::isMediaValid() const {
	if ( !mMediaQueryList ) {
		return true;
	}

	return mMediaQueryList->isUsed();
}

const MediaQueryList::ptr& StyleSheetStyle::getMediaQueryList() const {
	return mMediaQueryList;
}

bool StyleSheetStyle::isAtRule() const {
	return mAtRuleType != AtRuleType::None;
}

const AtRuleType& StyleSheetStyle::getAtRuleType() const {
	return mAtRuleType;
}

AtRuleType StyleSheetStyle::checkAtRule() {
	if ( mSelector.getName() == "@font-face" ) {
		return AtRuleType::FontFace;
	}

	return AtRuleType::None;
}

}}} // namespace EE::UI::CSS
