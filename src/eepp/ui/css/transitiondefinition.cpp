#include <eepp/core/string.hpp>
#include <eepp/ui/css/declarationparser.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/timingfunction.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>

namespace EE { namespace UI { namespace CSS {

using namespace DeclarationParser;

UnorderedMap<std::string, TransitionDefinition> TransitionDefinition::parseTransitionProperties(
	const std::vector<const StyleSheetProperty*>& styleSheetProperties ) {
	SmallVector<std::string, 4> properties;
	SmallVector<Time, 4> durations;
	SmallVector<Time, 4> delays;
	SmallVector<TimingFunction, 4> timingFunctions;
	TransitionsMap transitions;

	for ( const StyleSheetProperty* property : styleSheetProperties ) {
		if ( nullptr == property || nullptr == property->getPropertyDefinition() )
			continue;

		switch ( property->getPropertyDefinition()->getPropertyId() ) {
			case PropertyId::Transition:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					auto tokens = splitWhitespaceTokens<4>( item );
					if ( tokens.size() < 2 )
						return;
					TransitionDefinition transition;
					transition.property = lowerString( tokens[0] );
					transition.duration = parseTime( tokens[1] );
					if ( tokens.size() >= 3 ) {
						TimingFunction timing = TimingFunction::parse( tokens[2] );
						transition.timingFunction = timing.interpolation;
						transition.timingFunctionParameters.assign( timing.parameters.begin(),
																	timing.parameters.end() );
						if ( transition.timingFunction == Ease::None && tokens.size() == 3 )
							transition.delay = parseTime( tokens[2] );
						else if ( tokens.size() >= 4 )
							transition.delay = parseTime( tokens[3] );
					}
					transitions[transition.property] = std::move( transition );
				} );
				break;
			case PropertyId::TransitionDuration:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					durations.emplace_back( parseTime( item ) );
				} );
				break;
			case PropertyId::TransitionDelay:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					delays.emplace_back( parseTime( item ) );
				} );
				break;
			case PropertyId::TransitionTimingFunction:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					timingFunctions.emplace_back( TimingFunction::parse( item ) );
				} );
				break;
			case PropertyId::TransitionProperty:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					properties.emplace_back( lowerString( item ) );
				} );
				break;
			default:
				break;
		}
	}

	for ( std::size_t i = 0; i < properties.size(); ++i ) {
		TransitionDefinition transition;
		transition.property = properties[i];
		if ( !durations.empty() )
			transition.duration = durations[i % durations.size()];
		if ( !delays.empty() )
			transition.delay = delays[i % delays.size()];
		if ( !timingFunctions.empty() ) {
			const TimingFunction& timing = timingFunctions[i % timingFunctions.size()];
			transition.timingFunction = timing.interpolation;
			transition.timingFunctionParameters.assign( timing.parameters.begin(),
														timing.parameters.end() );
		}
		transitions[transition.property] = std::move( transition );
	}

	return transitions;
}

void ComputedTransitions::set( const ComputedTransitionDefinition& transition ) {
	for ( auto& current : mTransitions ) {
		if ( current.propertyNameHash == transition.propertyNameHash ) {
			current = transition;
			return;
		}
	}
	mTransitions.emplace_back( transition );
}

const ComputedTransitionDefinition*
ComputedTransitions::get( String::HashType propertyNameHash ) const {
	const ComputedTransitionDefinition* all = nullptr;
	for ( const auto& transition : mTransitions ) {
		if ( transition.propertyNameHash == propertyNameHash )
			return &transition;
		if ( transition.propertyNameHash == String::hash( "all" ) )
			all = &transition;
	}
	return all;
}

ComputedTransitions
ComputedTransitions::parse( const std::vector<const StyleSheetProperty*>& styleSheetProperties ) {
	SmallVector<String::HashType, 4> properties;
	SmallVector<Time, 4> durations;
	SmallVector<Time, 4> delays;
	SmallVector<TimingFunction, 4> timingFunctions;
	ComputedTransitions transitions;

	for ( const StyleSheetProperty* property : styleSheetProperties ) {
		if ( nullptr == property || nullptr == property->getPropertyDefinition() )
			continue;

		switch ( property->getPropertyDefinition()->getPropertyId() ) {
			case PropertyId::Transition:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					auto tokens = splitWhitespaceTokens<4>( item );
					if ( tokens.size() < 2 )
						return;
					ComputedTransitionDefinition transition;
					transition.propertyNameHash = lowerHash( tokens[0] );
					transition.duration = parseTime( tokens[1] );
					if ( tokens.size() >= 3 ) {
						TimingFunction timing = TimingFunction::parse( tokens[2] );
						transition.timingFunction = timing.interpolation;
						transition.timingFunctionParameters = std::move( timing.parameters );
						if ( transition.timingFunction == Ease::None && tokens.size() == 3 )
							transition.delay = parseTime( tokens[2] );
						else if ( tokens.size() >= 4 )
							transition.delay = parseTime( tokens[3] );
					}
					transitions.set( transition );
				} );
				break;
			case PropertyId::TransitionDuration:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					durations.emplace_back( parseTime( item ) );
				} );
				break;
			case PropertyId::TransitionDelay:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					delays.emplace_back( parseTime( item ) );
				} );
				break;
			case PropertyId::TransitionTimingFunction:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					timingFunctions.emplace_back( TimingFunction::parse( item ) );
				} );
				break;
			case PropertyId::TransitionProperty:
				forEachCommaItem( property->getValue(), [&]( std::string_view item ) {
					properties.emplace_back( lowerHash( item ) );
				} );
				break;
			default:
				break;
		}
	}

	for ( std::size_t i = 0; i < properties.size(); ++i ) {
		ComputedTransitionDefinition transition;
		transition.propertyNameHash = properties[i];
		if ( !durations.empty() )
			transition.duration = durations[i % durations.size()];
		if ( !delays.empty() )
			transition.delay = delays[i % delays.size()];
		if ( !timingFunctions.empty() ) {
			const TimingFunction& timing = timingFunctions[i % timingFunctions.size()];
			transition.timingFunction = timing.interpolation;
			transition.timingFunctionParameters = timing.parameters;
		}
		transitions.set( transition );
	}

	return transitions;
}

}}} // namespace EE::UI::CSS
