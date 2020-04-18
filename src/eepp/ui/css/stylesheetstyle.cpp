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

StyleSheetProperty StyleSheetStyle::getPropertyById( const Uint32& id ) const {
	auto it = mProperties.find( id );

	if ( it != mProperties.end() )
		return it->second;

	return StyleSheetProperty();
}

void StyleSheetStyle::setProperty( const StyleSheetProperty& property ) {
	if ( NULL != property.getPropertyDefinition() &&
		 property.getPropertyDefinition()->isIndexed() ) {
		// If the property being set is indexed we need to merge any other index set to the new
		// property set.
		StyleSheetProperty currentProperty = getPropertyById( property.getId() );
		std::vector<std::string> values;
		if ( currentProperty.isEmpty() ) {
			if ( property.getIndex() > 0 ) {
				for ( size_t i = 0; i < property.getIndex(); i++ ) {
					values.emplace_back( property.getPropertyDefinition()->getDefaultValue() );
				}
			}
			values.emplace_back( property.getValue() );
		} else {
			size_t maxIndex =
				eemax( currentProperty.getPropertyIndexCount(), (size_t)property.getIndex() );
			for ( size_t i = 0; i < maxIndex; i++ ) {
				if ( property.getIndex() == i ) {
					values.emplace_back( property.getValue() );
				} else if ( i < currentProperty.getPropertyIndexCount() ) {
					values.emplace_back( currentProperty.getPropertyIndex( i ).getValue() );
				} else {
					values.emplace_back( property.getPropertyDefinition()->getDefaultValue() );
				}
			}
		}
		currentProperty =
			StyleSheetProperty( property.getPropertyDefinition(), String::join( values, ',' ) );
		mProperties[property.getId()] = currentProperty;
	} else {
		mProperties[property.getId()] = property;
	}
}

void StyleSheetStyle::clearProperties() {
	mProperties.clear();
}

StyleSheetVariable StyleSheetStyle::getVariableByName( const std::string& name ) const {
	auto it = mVariables.find( String::hash( name ) );

	if ( it != mVariables.end() )
		return it->second;

	return StyleSheetVariable();
}

void StyleSheetStyle::setVariable( const StyleSheetVariable& variable ) {
	mVariables[variable.getNameHash()] = variable;
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
