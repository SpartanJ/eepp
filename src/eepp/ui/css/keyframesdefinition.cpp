#include <eepp/ui/css/keyframesdefinition.hpp>

namespace EE { namespace UI { namespace CSS {

KeyframesDefinition
KeyframesDefinition::parseKeyframes( const std::string& name,
									 const StyleSheetStyleVector& keyframeBlocks ) {
	KeyframesDefinition def;
	def.name = name;

	for ( const auto& block : keyframeBlocks ) {
		std::string blockName( String::toLower( String::trim( block.getSelector().getName() ) ) );
		Float blockTime = 0.f;

		if ( blockName == "from" ) {
			blockTime = 0.f;
		} else if ( blockName == "to" ) {
			blockTime = 1.f;
		} else if ( blockName[blockName.size() - 1] == '%' ) {
			std::string blockNum( blockName.substr( 0, blockName.size() - 1 ) );
			Float num = 0.f;
			if ( String::fromString( num, blockNum ) ) {
				blockTime = num / 100.f;
			}
		} else {
			continue;
		}

		def.keyframeBlocks[blockTime] = { blockTime, block.getProperties() };
	}

	return def;
}

const std::map<Float, KeyframesDefinition::KeyframeBlock>&
KeyframesDefinition::getKeyframeBlocks() const {
	return keyframeBlocks;
}

const std::string& KeyframesDefinition::getName() const {
	return name;
}

}}} // namespace EE::UI::CSS
