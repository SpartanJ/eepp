#include <algorithm>
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

std::string StyleSheetStyle::build( bool emmitMediaQueryStart, bool emmitMediaQueryEnd ) {
	std::string css;

	if ( emmitMediaQueryStart && mMediaQueryList && !mMediaQueryList->getQueryString().empty() )
		css += mMediaQueryList->getQueryString() + " {\n\n";

	css += mSelector.getName() + " {\n";

	for ( auto& it : mVariables )
		css += "\t" + it.second.getName() + ": " + it.second.getValue() + ";\n";

	for ( auto& it : mProperties ) {
		StyleSheetProperty& prop = it.second;

		css += "\t" + prop.getName() + ": " + prop.getValue() + ";\n";
	}

	css += "}\n\n";

	if ( emmitMediaQueryEnd && mMediaQueryList && !mMediaQueryList->getQueryString().empty() )
		css += "}\n\n";

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

bool StyleSheetStyle::updatePropertyValue( const std::string& name, const std::string& value ) {
	bool updated = false;
	for ( auto& prop : mProperties ) {
		if ( prop.second.getName() == name ) {
			prop.second.setValue( value, true );
			updated = true;
		}
	}
	return updated;
}

const StyleSheetVariables& StyleSheetStyle::getVariables() const {
	return mVariables;
}

const StyleSheetProperty* StyleSheetStyle::getPropertyById( const PropertyId& id ) const {
	for ( auto& prop : mProperties ) {
		if ( NULL != prop.second.getPropertyDefinition() &&
			 prop.second.getPropertyDefinition()->getPropertyId() == id ) {
			return &prop.second;
		}
	}

	return nullptr;
}

const StyleSheetProperty*
StyleSheetStyle::getPropertyByDefinition( const PropertyDefinition* def ) const {
	for ( auto& prop : mProperties ) {
		if ( NULL != prop.second.getPropertyDefinition() &&
			 prop.second.getPropertyDefinition() == def ) {
			return &prop.second;
		}
	}

	return nullptr;
}

StyleSheetProperty* StyleSheetStyle::getPropertyById( const Uint32& id ) {
	auto it = mProperties.find( id );

	if ( it != mProperties.end() )
		return &it->second;

	return nullptr;
}

void StyleSheetStyle::setProperty( const StyleSheetProperty& property ) {
	if ( NULL != property.getPropertyDefinition() &&
		 property.getPropertyDefinition()->isIndexed() ) {
		// If the property being set is indexed we need to merge any other index set to the new
		// property set.
		const StyleSheetProperty* currentProperty = getPropertyById( property.getId() );
		std::vector<std::string> values;
		if ( nullptr == currentProperty ) {
			if ( property.getIndex() > 0 ) {
				for ( size_t i = 0; i < property.getIndex(); i++ ) {
					values.emplace_back( property.getPropertyDefinition()->getDefaultValue() );
				}
			}
			values.emplace_back( property.getValue() );
		} else {
			size_t maxIndex =
				eemax( currentProperty->getPropertyIndexCount(), (size_t)property.getIndex() );
			for ( size_t i = 0; i < maxIndex; i++ ) {
				if ( property.getIndex() == i ) {
					values.emplace_back( property.getValue() );
				} else if ( i < currentProperty->getPropertyIndexCount() ) {
					values.emplace_back( currentProperty->getPropertyIndex( i ).getValue() );
				} else {
					values.emplace_back( property.getPropertyDefinition()->getDefaultValue() );
				}
			}
		}
		mProperties[property.getId()] =
			StyleSheetProperty( property.getPropertyDefinition(), String::join( values, ',' ) );
	} else {
		mProperties[property.getId()] = property;
	}
}

void StyleSheetStyle::clearProperties() {
	mProperties.clear();
}

bool StyleSheetStyle::hasProperties() const {
	return !mProperties.empty();
}

bool StyleSheetStyle::hasProperty( PropertyId id ) const {
	return std::find_if( mProperties.begin(), mProperties.end(), [&id]( const auto& prop ) {
			   return prop.second.getPropertyDefinition() &&
					  prop.second.getPropertyDefinition()->getPropertyId() == id;
		   } ) != mProperties.end();
}

bool StyleSheetStyle::hasProperty( const std::string& name ) const {
	return std::find_if( mProperties.begin(), mProperties.end(), [&name]( const auto& prop ) {
			   return prop.second.getPropertyDefinition() &&
					  prop.second.getPropertyDefinition()->getName() == name;
		   } ) != mProperties.end();
}

bool StyleSheetStyle::hasVariables() const {
	return !mVariables.empty();
}

bool StyleSheetStyle::hasVariable( const std::string& name ) const {
	return !getVariableByName( name ).isEmpty();
}

StyleSheetVariable StyleSheetStyle::getVariableByName( const std::string& name ) const {
	auto it = mVariables.find( String::hash( name ) );

	if ( it != mVariables.end() )
		return it->second;

	return StyleSheetVariable();
}

void StyleSheetStyle::setVariable( const StyleSheetVariable& variable ) {
	mVariables[variable.getNameHash()] = variable;
	mVariables[variable.getNameHash()].setSpecificity( mSelector.getSpecificity() );
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

const Uint32& StyleSheetStyle::getMarker() const {
	return mMarker;
}

void StyleSheetStyle::setMarker( const Uint32& marker ) {
	mMarker = marker;
	if ( mMediaQueryList )
		mMediaQueryList->setMarker( marker );
}

AtRuleType StyleSheetStyle::checkAtRule() {
	if ( mSelector.getName() == "@font-face" ) {
		return AtRuleType::FontFace;
	} else if ( mSelector.getName() == "@glyph-icon" ) {
		return AtRuleType::GlyphIcon;
	}

	return AtRuleType::None;
}

}}} // namespace EE::UI::CSS
