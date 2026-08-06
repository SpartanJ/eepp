#include <cctype>
#include <eepp/core/string.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/timingfunction.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>

namespace EE { namespace UI { namespace CSS {

namespace {

template <typename Callback> void forEachCommaItem( std::string_view value, Callback&& callback ) {
	System::FunctionString::forEachParameter( value, [&]( std::string_view item, bool ) {
		callback( item );
		return true;
	} );
}

SmallVector<std::string_view, 4> splitTransitionTokens( std::string_view value ) {
	SmallVector<std::string_view, 4> tokens;
	std::size_t start = 0;
	int parenthesisDepth = 0;
	for ( std::size_t i = 0; i <= value.size(); ++i ) {
		if ( i < value.size() ) {
			if ( value[i] == '(' )
				++parenthesisDepth;
			else if ( value[i] == ')' && parenthesisDepth > 0 )
				--parenthesisDepth;
		}
		if ( i == value.size() ||
			 ( std::isspace( static_cast<unsigned char>( value[i] ) ) && parenthesisDepth == 0 ) ) {
			if ( i > start )
				tokens.emplace_back( value.substr( start, i - start ) );
			while ( i + 1 < value.size() &&
					std::isspace( static_cast<unsigned char>( value[i + 1] ) ) )
				++i;
			start = i + 1;
		}
	}
	return tokens;
}

Time parseTime( std::string_view value ) {
	value = String::trim( value, " \t\n\r\f\v" );
	auto lower = []( char character ) {
		return character >= 'A' && character <= 'Z' ? character + ( 'a' - 'A' ) : character;
	};
	bool milliseconds = value.size() >= 2 && lower( value[value.size() - 2] ) == 'm' &&
						lower( value.back() ) == 's';
	bool seconds = !milliseconds && !value.empty() && lower( value.back() ) == 's';
	bool minutes = !milliseconds && !seconds && !value.empty() && lower( value.back() ) == 'm';
	std::size_t numberLength = value.size() - ( milliseconds ? 2 : ( seconds || minutes ? 1 : 0 ) );
	double number = 0;
	const char* numberStart = value.data();
	if ( numberLength > 0 && *numberStart == '+' ) {
		++numberStart;
		--numberLength;
	}
	if ( !String::fromString( number, std::string_view{ numberStart, numberLength } ) )
		return Time::Zero;
	if ( milliseconds )
		return Milliseconds( number );
	if ( minutes )
		return Minutes( number );
	return Seconds( number );
}

String::HashType lowerHash( std::string_view value ) {
	return String::hashToLower( value.data(), static_cast<Int64>( value.size() ) );
}

std::string lowerString( std::string_view value ) {
	std::string result{ value };
	String::toLowerInPlace( result );
	return result;
}

} // namespace

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
					auto tokens = splitTransitionTokens( item );
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
					auto tokens = splitTransitionTokens( item );
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
