#include <eepp/ui/css/elementdefinition.hpp>

namespace EE { namespace UI { namespace CSS {

ElementDefinition::ElementDefinition( const StyleSheetStyleVector& styleSheetStyles ) {
	// Initialises the element definition from the list of style sheet nodes.
	for ( size_t i = 0; i < styleSheetStyles.size(); ++i ) {
		const StyleSheetProperties& properties = styleSheetStyles[i]->getProperties();

		for ( auto iterator = properties.begin(); iterator != properties.end(); ++iterator ) {
			const StyleSheetProperty& property = iterator->second;
			const auto& it = mProperties.find( property.getId() );

			if ( it == mProperties.end() ||
				 property.getSpecificity() >= it->second.getSpecificity() ) {
				mProperties[property.getId()] = property;
			}
		}
	}

	for ( auto& property : mProperties )
		mPropertyIds.insert( property.first );
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

}}} // namespace EE::UI::CSS
