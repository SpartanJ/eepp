#include <eepp/core/string.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/timingfunction.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>

namespace EE { namespace UI { namespace CSS {

UnorderedMap<std::string, TransitionDefinition> TransitionDefinition::parseTransitionProperties(
	const std::vector<const StyleSheetProperty*>& styleSheetProperties ) {
	std::vector<std::string> properties;
	std::vector<Time> durations;
	std::vector<Time> delays;
	std::vector<Ease::Interpolation> timingFunctions;
	std::vector<std::vector<double>> timingFunctionParameters;
	TransitionsMap transitions;

	for ( auto& prop : styleSheetProperties ) {
		if ( prop->getPropertyDefinition() == NULL )
			continue;

		const PropertyDefinition* propDef = prop->getPropertyDefinition();

		if ( propDef->getPropertyId() == PropertyId::Transition ) {
			auto strTransitions = String::split( prop->getValue(), ",", ",", "()" );

			for ( auto tit = strTransitions.begin(); tit != strTransitions.end(); ++tit ) {
				auto strTransition = String::trim( *tit );
				auto splitTransition = String::split( strTransition, " ", "", "()" );

				if ( !splitTransition.empty() ) {
					TransitionDefinition transitionDef;

					if ( splitTransition.size() >= 2 ) {
						std::string property = String::trim( splitTransition[0] );
						String::toLowerInPlace( property );

						Time duration = StyleSheetProperty( prop->getName(),
															String::toLower( splitTransition[1] ) )
											.asTime();

						transitionDef.property = property;
						transitionDef.duration = duration;

						if ( splitTransition.size() >= 3 ) {
							TimingFunction tf( TimingFunction::parse( splitTransition[2] ) );
							transitionDef.timingFunction = std::move( tf.interpolation );
							transitionDef.timingFunctionParameters = std::move( tf.parameters );

							if ( transitionDef.timingFunction == Ease::None &&
								 splitTransition.size() == 3 ) {
								transitionDef.delay =
									StyleSheetProperty( prop->getName(),
														String::toLower( splitTransition[2] ) )
										.asTime();
							} else if ( splitTransition.size() >= 4 ) {
								transitionDef.delay =
									StyleSheetProperty( prop->getName(),
														String::toLower( splitTransition[3] ) )
										.asTime();
							}
						}

						transitions[transitionDef.getProperty()] = transitionDef;
					}
				}
			}
		} else if ( propDef->getPropertyId() == PropertyId::TransitionDuration ) {
			auto strDurations = String::split( prop->getValue(), ',' );

			for ( auto dit = strDurations.begin(); dit != strDurations.end(); ++dit ) {
				std::string duration( String::trim( *dit ) );
				String::toLowerInPlace( duration );
				durations.push_back( StyleSheetProperty( prop->getName(), duration ).asTime() );
			}
		} else if ( propDef->getPropertyId() == PropertyId::TransitionDelay ) {
			auto strDelays = String::split( prop->getValue(), ',' );

			for ( auto dit = strDelays.begin(); dit != strDelays.end(); ++dit ) {
				std::string delay( String::trim( *dit ) );
				String::toLowerInPlace( delay );
				delays.push_back( StyleSheetProperty( prop->getName(), delay ).asTime() );
			}
		} else if ( propDef->getPropertyId() == PropertyId::TransitionTimingFunction ) {
			auto strTimingFuncs = String::split( prop->getValue(), ",", "", "()" );

			for ( auto dit = strTimingFuncs.begin(); dit != strTimingFuncs.end(); ++dit ) {
				TimingFunction tf( TimingFunction::parse( *dit ) );
				timingFunctions.emplace_back( tf.interpolation );
				timingFunctionParameters.emplace_back( tf.parameters );
			}
		} else if ( propDef->getPropertyId() == PropertyId::TransitionProperty ) {
			auto strProperties = String::split( prop->getValue(), ',' );

			for ( auto dit = strProperties.begin(); dit != strProperties.end(); ++dit ) {
				std::string property( String::trim( *dit ) );
				String::toLowerInPlace( property );
				properties.push_back( property );
			}
		}
	}

	for ( size_t i = 0; i < properties.size(); i++ ) {
		const std::string& property = properties.at( i );
		TransitionDefinition transitionDef;

		transitionDef.property = property;

		if ( !durations.empty() )
			transitionDef.duration = durations[i % durations.size()];

		if ( !delays.empty() )
			transitionDef.delay = delays[i % delays.size()];

		if ( !timingFunctions.empty() ) {
			size_t idx = !delays.empty() ? i % delays.size() : 0;
			transitionDef.timingFunction = timingFunctions[idx];
			transitionDef.timingFunctionParameters = timingFunctionParameters[idx];
		}

		transitions[property] = transitionDef;
	}

	return transitions;
}

}}} // namespace EE::UI::CSS
