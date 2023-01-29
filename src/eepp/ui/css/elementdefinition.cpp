#include <eepp/ui/css/elementdefinition.hpp>

namespace EE { namespace UI { namespace CSS {

ElementDefinition::ElementDefinition( const StyleSheetStyleVector& styleSheetStyles ) :
	mStyles( styleSheetStyles ), mStructurallyVolatile( false ) {
	refresh();
}

StyleSheetProperty* ElementDefinition::getProperty( const Uint32& id ) {
	auto it = mProperties.find( id );
	return it != mProperties.end() ? &it->second : nullptr;
}

const PropertyIdSet& ElementDefinition::getPropertyIds() const {
	return mPropertyIds;
}

const StyleSheetProperties& ElementDefinition::getProperties() const {
	return mProperties;
}

const std::vector<const StyleSheetProperty*>& ElementDefinition::getTransitionProperties() const {
	return mTransitionProperties;
}

const std::vector<const StyleSheetProperty*>& ElementDefinition::getAnimationProperties() const {
	return mAnimationProperties;
}

const StyleSheetVariables& ElementDefinition::getVariables() const {
	return mVariables;
}

bool ElementDefinition::isStructurallyVolatile() const {
	return mStructurallyVolatile;
}

const StyleSheetStyleVector& ElementDefinition::getStyles() const {
	return mStyles;
}

void ElementDefinition::refresh() {
	mProperties.clear();
	mTransitionProperties.clear();
	mAnimationProperties.clear();
	mPropertyIds.clear();
	mStructurallyVolatile = false;
	for ( auto& styleSheetStyle : mStyles ) {
		const StyleSheetProperties& properties = styleSheetStyle->getProperties();

		if ( styleSheetStyle->getSelector().isStructurallyVolatile() )
			mStructurallyVolatile = true;

		for ( auto iterator = properties.begin(); iterator != properties.end(); ++iterator ) {
			const StyleSheetProperty& property = iterator->second;
			const auto& it = mProperties.find( property.getId() );

			if ( it == mProperties.end() ||
				 property.getSpecificity() >= it->second.getSpecificity() ) {
				mProperties[property.getId()] = property;
			}

			if ( String::startsWith( property.getName(), "transition" ) )
				mTransitionProperties.push_back( &property );
			else if ( String::startsWith( property.getName(), "animation" ) )
				mAnimationProperties.push_back( &property );
		}

		findVariables( styleSheetStyle );
	}

	for ( auto& property : mProperties )
		mPropertyIds.insert( property.first );
}

void ElementDefinition::findVariables( const StyleSheetStyle* style ) {
	for ( const auto& vars : style->getVariables() ) {
		const StyleSheetVariable& variable = vars.second;
		const auto& it = mVariables.find( variable.getNameHash() );

		if ( it == mVariables.end() || variable.getSpecificity() >= it->second.getSpecificity() ) {
			mVariables[variable.getNameHash()] = variable;
		}
	}
}

}}} // namespace EE::UI::CSS
