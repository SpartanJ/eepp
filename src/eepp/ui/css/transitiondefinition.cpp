#include <eepp/core/string.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>

namespace EE { namespace UI { namespace CSS {

std::map<std::string, TransitionDefinition> TransitionDefinition::parseTransitionProperties(
	const std::vector<StyleSheetProperty>& styleSheetProperties ) {
	std::vector<std::string> properties;
	std::vector<Time> durations;
	std::vector<Time> delays;
	std::vector<Ease::Interpolation> timingFunctions;
	TransitionsMap transitions;

	for ( auto& prop : styleSheetProperties ) {
		if ( prop.getName() == "transition" ) {
			auto strTransitions = String::split( prop.getValue(), ',' );

			for ( auto tit = strTransitions.begin(); tit != strTransitions.end(); ++tit ) {
				auto strTransition = String::trim( *tit );
				auto splitTransition = String::split( strTransition, ' ' );

				if ( !splitTransition.empty() ) {
					TransitionDefinition transitionDef;

					if ( splitTransition.size() >= 2 ) {
						std::string property = String::trim( splitTransition[0] );
						String::toLowerInPlace( property );

						Time duration = StyleSheetProperty( prop.getName(),
															String::toLower( splitTransition[1] ) )
											.asTime();

						transitionDef.property = property;
						transitionDef.duration = duration;

						if ( splitTransition.size() >= 3 ) {
							transitionDef.timingFunction =
								Ease::fromName( String::toLower( splitTransition[2] ) );

							if ( transitionDef.timingFunction == Ease::Linear &&
								 splitTransition[2] != "linear" && splitTransition.size() == 3 ) {
								transitionDef.delay =
									StyleSheetProperty( prop.getName(),
														String::toLower( splitTransition[2] ) )
										.asTime();
							} else if ( splitTransition.size() >= 4 ) {
								transitionDef.delay =
									StyleSheetProperty( prop.getName(),
														String::toLower( splitTransition[3] ) )
										.asTime();
							}
						}

						transitions[transitionDef.getProperty()] = transitionDef;
					}
				}
			}
		} else if ( prop.getName() == "transitionduration" ||
					prop.getName() == "transition-duration" ) {
			auto strDurations = String::split( prop.getValue(), ',' );

			for ( auto dit = strDurations.begin(); dit != strDurations.end(); ++dit ) {
				std::string duration( String::trim( *dit ) );
				String::toLowerInPlace( duration );
				durations.push_back( StyleSheetProperty( prop.getName(), duration ).asTime() );
			}
		} else if ( prop.getName() == "transitiondelay" || prop.getName() == "transition-delay" ) {
			auto strDelays = String::split( prop.getValue(), ',' );

			for ( auto dit = strDelays.begin(); dit != strDelays.end(); ++dit ) {
				std::string delay( String::trim( *dit ) );
				String::toLowerInPlace( delay );
				delays.push_back( StyleSheetProperty( prop.getName(), delay ).asTime() );
			}
		} else if ( prop.getName() == "transitiontimingfunction" ||
					prop.getName() == "transition-timing-function" ) {
			auto strTimingFuncs = String::split( prop.getValue(), ',' );

			for ( auto dit = strTimingFuncs.begin(); dit != strTimingFuncs.end(); ++dit ) {
				std::string timingFunction( String::trim( *dit ) );
				String::toLowerInPlace( timingFunction );
				timingFunctions.push_back( Ease::fromName( timingFunction ) );
			}
		} else if ( prop.getName() == "transitionproperty" ||
					prop.getName() == "transition-property" ) {
			auto strProperties = String::split( prop.getValue(), ',' );

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

		if ( durations.size() < i ) {
			transitionDef.duration = durations[i];
		} else if ( !durations.empty() ) {
			transitionDef.duration = durations[0];
		}

		if ( delays.size() < i ) {
			transitionDef.delay = delays[i];
		} else if ( !delays.empty() ) {
			transitionDef.delay = delays[0];
		}

		if ( timingFunctions.size() < i ) {
			transitionDef.timingFunction = timingFunctions[i];
		} else if ( !delays.empty() ) {
			transitionDef.timingFunction = timingFunctions[0];
		}

		transitions[property] = transitionDef;
	}

	return transitions;
}

}}} // namespace EE::UI::CSS
