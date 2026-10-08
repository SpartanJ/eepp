#include <eepp/ui/css/animationdefinition.hpp>
#include <eepp/ui/css/declarationparser.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/timingfunction.hpp>

namespace EE { namespace UI { namespace CSS {

namespace {

bool isTime( std::string_view value ) {
	value = String::trim( value, " \t\n\r\f\v" );
	if ( value.empty() )
		return false;
	auto lower = []( char character ) {
		return character >= 'A' && character <= 'Z' ? character + ( 'a' - 'A' ) : character;
	};
	std::size_t suffixSize = 0;
	if ( value.size() >= 2 && lower( value[value.size() - 2] ) == 'm' &&
		 lower( value.back() ) == 's' )
		suffixSize = 2;
	else if ( lower( value.back() ) == 's' || lower( value.back() ) == 'm' )
		suffixSize = 1;
	if ( 0 == suffixSize || value.size() == suffixSize )
		return false;
	double number = 0;
	std::string_view numberView = value.substr( 0, value.size() - suffixSize );
	if ( !numberView.empty() && numberView.front() == '+' )
		numberView.remove_prefix( 1 );
	return String::fromString( number, numberView );
}

} // namespace

UnorderedMap<std::string, AnimationDefinition> AnimationDefinition::parseAnimationProperties(
	const std::vector<const StyleSheetProperty*>& stylesheetProperties ) {
	AnimationsMap animations;
	SmallVector<std::string_view, 4> names;
	SmallVector<Time, 4> durations;
	SmallVector<Time, 4> delays;
	SmallVector<Int32, 4> iterations;
	SmallVector<TimingFunction, 4> timingFunctions;
	SmallVector<AnimationDirection, 4> directions;
	SmallVector<AnimationFillMode, 4> fillModes;
	SmallVector<Uint8, 4> pausedStates;

	for ( auto& prop : stylesheetProperties ) {
		if ( prop->getPropertyDefinition() == NULL )
			continue;

		const PropertyDefinition* propDef = prop->getPropertyDefinition();

		switch ( propDef->getPropertyId() ) {
			case PropertyId::Animation: {
				for ( size_t i = 0; i < prop->getPropertyIndexCount(); i++ ) {
					const StyleSheetProperty& iProp = prop->getPropertyIndex( i );
					auto parts = DeclarationParser::splitWhitespaceTokens<8>( iProp.getValue() );

					if ( parts.size() >= 2 ) {
						AnimationDefinition animationDef;
						bool durationSet = false;

						for ( std::string_view part : parts ) {
							const auto hash = DeclarationParser::lowerHash( part );

							if ( isDirectionStringView( part ) ) {
								animationDef.setDirection( directionFromStringView( part ) );
							} else if ( isAnimationFillModeStringView( part ) ) {
								animationDef.setFillMode( fillModeFromStringView( part ) );
							} else if ( hash == String::hash( "infinite" ) ) {
								animationDef.setIterations( -1 );
							} else if ( hash == String::hash( "paused" ) ) {
								animationDef.setPaused( true );
							} else if ( hash == String::hash( "running" ) ) {
								animationDef.setPaused( false );
							} else if ( TimingFunction tf = TimingFunction::parse( part );
										Ease::Interpolation::None != tf.interpolation ) {
								animationDef.setTimingFunction( tf.interpolation );
								animationDef.setTimingFunctionParameters( tf.parameters );
							} else if ( isTime( part ) ) {
								if ( durationSet ) {
									animationDef.setDelay( DeclarationParser::parseTime( part ) );
								} else {
									animationDef.setDuration(
										DeclarationParser::parseTime( part ) );
									durationSet = true;
								}
							} else if ( int iterations = 1;
										String::fromString( iterations, part ) ) {
								animationDef.setIterations( iterations );
							} else {
								animationDef.setNameView( part );
							}
						}

						animations.insert_or_assign( animationDef.getName(),
													 std::move( animationDef ) );
					}
				}
				return animations;
				break;
			}
			case PropertyId::AnimationName:
			case PropertyId::AnimationDelay:
			case PropertyId::AnimationDuration:
			case PropertyId::AnimationFillMode:
			case PropertyId::AnimationPlayState:
			case PropertyId::AnimationDirection:
			case PropertyId::AnimationIterationCount:
			case PropertyId::AnimationTimingFunction: {
				for ( size_t i = 0; i < prop->getPropertyIndexCount(); i++ ) {
					const StyleSheetProperty& iProp = prop->getPropertyIndex( i );
					const std::string_view val =
						String::trim( std::string_view{ iProp.getValue() }, " \t\n\r\f\v" );
					const auto hash = DeclarationParser::lowerHash( val );
					switch ( propDef->getPropertyId() ) {
						case PropertyId::AnimationName:
							names.emplace_back( iProp.getValue() );
							break;
						case PropertyId::AnimationDelay:
							delays.emplace_back( DeclarationParser::parseTime( val ) );
							break;
						case PropertyId::AnimationDuration:
							durations.emplace_back( DeclarationParser::parseTime( val ) );
							break;
						case PropertyId::AnimationFillMode:
							fillModes.emplace_back( fillModeFromStringView( val ) );
							break;
						case PropertyId::AnimationPlayState:
							pausedStates.emplace_back( hash == String::hash( "paused" ) );
							break;
						case PropertyId::AnimationDirection:
							directions.emplace_back( directionFromStringView( val ) );
							break;
						case PropertyId::AnimationIterationCount: {
							int iVal;
							if ( hash == String::hash( "infinite" ) ) {
								iterations.emplace_back( -1 );
							} else if ( String::fromString( iVal, val ) && iVal >= -1 ) {
								iterations.emplace_back( iVal );
							}
							break;
						}
						case PropertyId::AnimationTimingFunction: {
							timingFunctions.emplace_back( TimingFunction::parse( val ) );
							break;
						}
						default:
							break;
					}
				}
				break;
			}
			default:
				break;
		}
	}

	for ( size_t i = 0; i < names.size(); i++ ) {
		AnimationDefinition animationDef;
		animationDef.setNameView( names[i] );

		if ( !delays.empty() )
			animationDef.setDelay( delays[i % delays.size()] );

		if ( !durations.empty() )
			animationDef.setDuration( durations[i % durations.size()] );

		if ( !fillModes.empty() )
			animationDef.setFillMode( fillModes[i % fillModes.size()] );

		if ( !directions.empty() )
			animationDef.setDirection( directions[i % directions.size()] );

		if ( !pausedStates.empty() )
			animationDef.setPaused( pausedStates[i % pausedStates.size()] );

		if ( !iterations.empty() )
			animationDef.setIterations( iterations[i % iterations.size()] );

		if ( !timingFunctions.empty() ) {
			const TimingFunction& timing = timingFunctions[i % timingFunctions.size()];
			animationDef.setTimingFunction( timing.interpolation );
			animationDef.setTimingFunctionParameters( timing.parameters );
		}

		animations.insert_or_assign( animationDef.getName(), std::move( animationDef ) );
	}

	return animations;
}

bool AnimationDefinition::isDirectionString( const std::string str ) {
	return isDirectionStringView( str );
}

bool AnimationDefinition::isDirectionStringView( std::string_view str ) {
	String::HashType id = DeclarationParser::lowerHash( str );
	return id == AlternateReverse || id == Alternate || id == Reverse || id == Normal;
}

AnimationDefinition::AnimationDirection
AnimationDefinition::directionFromString( std::string str ) {
	return directionFromStringView( str );
}

AnimationDefinition::AnimationDirection
AnimationDefinition::directionFromStringView( std::string_view str ) {
	str = String::trim( str, " \t\n\r\f\v" );
	switch ( DeclarationParser::lowerHash( str ) ) {
		case AlternateReverse:
			return AlternateReverse;
		case Alternate:
			return Alternate;
		case Reverse:
			return Reverse;
		case Normal:
		default:
			return Normal;
	}
}

bool AnimationDefinition::isAnimationFillModeString( const std::string& str ) {
	return isAnimationFillModeStringView( str );
}

bool AnimationDefinition::isAnimationFillModeStringView( std::string_view str ) {
	Uint32 id = DeclarationParser::lowerHash( str );
	return id == None || id == Forwards || id == Backwards || id == Both;
}

AnimationDefinition::AnimationFillMode AnimationDefinition::fillModeFromString( std::string str ) {
	return fillModeFromStringView( str );
}

AnimationDefinition::AnimationFillMode
AnimationDefinition::fillModeFromStringView( std::string_view str ) {
	str = String::trim( str, " \t\n\r\f\v" );
	switch ( DeclarationParser::lowerHash( str ) ) {
		case None:
			return None;
		case Forwards:
			return Forwards;
		case Backwards:
			return Backwards;
		case Both:
		default:
			return Both;
	}
}

AnimationDefinition::AnimationDefinition() {}

const AnimationDefinition::AnimationDirection& AnimationDefinition::getDirection() const {
	return mDirection;
}

const bool& AnimationDefinition::isPaused() const {
	return mPaused;
}

const Int32& AnimationDefinition::getIterations() const {
	return mIterations;
}

const std::string& AnimationDefinition::getName() const {
	return mName;
}

const Time& AnimationDefinition::getDelay() const {
	return mDelay;
}

const Time& AnimationDefinition::getDuration() const {
	return mDuration;
}

const Ease::Interpolation& AnimationDefinition::getTimingFunction() const {
	return mTimingFunction;
}

void AnimationDefinition::setName( const std::string& value ) {
	setNameView( value );
}

void AnimationDefinition::setNameView( std::string_view value ) {
	mName.assign( value );
	mId = String::hash( mName );
}

void AnimationDefinition::setFillMode( const AnimationFillMode& value ) {
	mFillMode = value;
}

void AnimationDefinition::setPaused( bool value ) {
	mPaused = value;
}

const String::HashType& AnimationDefinition::getId() const {
	return mId;
}

const std::vector<double>& AnimationDefinition::getTimingFunctionParameters() const {
	if ( !mTimingFunctionParametersVectorValid ) {
		mTimingFunctionParametersVector.assign( mTimingFunctionParameters.begin(),
												mTimingFunctionParameters.end() );
		mTimingFunctionParametersVectorValid = true;
	}
	return mTimingFunctionParametersVector;
}

const TimingFunction::Parameters& AnimationDefinition::getTimingFunctionParametersInline() const {
	return mTimingFunctionParameters;
}

void AnimationDefinition::setTimingFunctionParameters(
	const std::vector<double>& timingFunctionParameters ) {
	mTimingFunctionParameters.assign( timingFunctionParameters.begin(),
									  timingFunctionParameters.end() );
	mTimingFunctionParametersVectorValid = false;
}

void AnimationDefinition::setTimingFunctionParameters(
	const TimingFunction::Parameters& timingFunctionParameters ) {
	mTimingFunctionParameters = timingFunctionParameters;
	mTimingFunctionParametersVectorValid = false;
}

const AnimationDefinition::AnimationFillMode& AnimationDefinition::getFillMode() const {
	return mFillMode;
}

void AnimationDefinition::setDirection( const AnimationDirection& value ) {
	mDirection = value;
}

void AnimationDefinition::setTimingFunction( const Ease::Interpolation& value ) {
	mTimingFunction = value;
}

void AnimationDefinition::setIterations( const Int32& value ) {
	mIterations = value;
}

void AnimationDefinition::setDuration( const Time& value ) {
	mDuration = value;
}

void AnimationDefinition::setDelay( const Time& value ) {
	mDelay = value;
}

}}} // namespace EE::UI::CSS
