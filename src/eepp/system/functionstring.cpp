#include <eepp/core/debug.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/functionstring.hpp>

namespace EE { namespace System {

#include <concepts>
#include <string>

template <AllowedFunctionString StringType>
FunctionString FunctionString::parse( StringType function ) {
	using CharType = typename StringType::value_type;

	size_t funcSep = function.find( '(' );
	if ( funcSep == StringType::npos )
		return FunctionString( "", {}, {} );

	auto funcName = String::trim( function.substr( 0, funcSep ) );
	Parameters funcParameters;
	TypeStringVector typeStringData;

	auto parametersString = function.substr( funcSep + 1 );
	size_t paramClose = parametersString.find_last_of( ')' );
	if ( paramClose == StringType::npos )
		return FunctionString( "", {}, {} );

	parametersString = parametersString.substr( 0, paramClose );

	bool stateParsingString = false;
	std::basic_string<CharType> buffer;
	CharType prevChar = 0;

	bool currentParamIsString = false;
	int parenDepth = 0;

	auto pushBufferToParams = [&]() {
		if constexpr ( std::same_as<CharType, char> ) {
			if ( !currentParamIsString )
				String::trimInPlace( buffer );
			funcParameters.push_back( buffer );
		} else {
			std::string utf8Buffer = String( buffer ).toUtf8();
			if ( !currentParamIsString )
				String::trimInPlace( utf8Buffer );
			funcParameters.push_back( utf8Buffer );
		}
		typeStringData.push_back( currentParamIsString );
		buffer.clear();
		currentParamIsString = false;
	};

	for ( size_t i = 0; i < parametersString.length(); ++i ) {

		CharType c = parametersString[i];

		if ( !stateParsingString ) {
			if ( c == '(' ) {
				parenDepth++;
				buffer += c;
			} else if ( c == ')' ) {
				if ( parenDepth == 0 )
					break;
				if ( parenDepth > 0 )
					parenDepth--;
				buffer += c;
			} else if ( c == ',' ) {
				if ( parenDepth == 0 ) {
					if ( !buffer.empty() ) {
						pushBufferToParams();
					}
				} else {
					buffer += c;
				}
			} else if ( c == '"' ) {
				stateParsingString = true;
				if ( parenDepth == 0 && buffer.empty() ) {
					currentParamIsString = true;
				} else {
					buffer += c;
				}
			} else if ( c == ' ' ) {
				if ( buffer.empty() || currentParamIsString )
					continue;
				else
					buffer += c;
			} else {
				buffer += c;
			}
		} else {
			if ( c == '\\' && i + 1 < parametersString.length() &&
				 parametersString[i + 1] == '"' ) {
			} else if ( prevChar != '\\' && c == '"' ) {
				stateParsingString = false;
				if ( !currentParamIsString ) {
					buffer += c;
				}
			} else {
				buffer += c;
			}
			prevChar = c;
		}
	}

	if ( !buffer.empty() )
		pushBufferToParams();

	if constexpr ( std::same_as<StringType, std::string> ) {
		return FunctionString( std::string{ funcName }, funcParameters, typeStringData );
	} else {
		return FunctionString( String( funcName ).toUtf8(), funcParameters, typeStringData );
	}
}

FunctionString FunctionString::parse( std::string_view function ) {
	return parse<std::string_view>( function );
}

FunctionString FunctionString::parse( String::View function ) {
	return parse<String::View>( function );
}

FunctionString::FunctionString( const std::string& name, const Parameters& parameters,
								const TypeStringVector& typeStringData ) :
	name( name ), parameters( parameters ), typeStringData( typeStringData ) {}

FunctionString::FunctionString( const std::string& name, Parameters&& parameters,
								TypeStringVector&& typeStringData ) :
	name( name ),
	parameters( std::move( parameters ) ),
	typeStringData( std::move( typeStringData ) ) {}

const std::string& FunctionString::getName() const {
	return name;
}

const FunctionString::Parameters& FunctionString::getParameters() const {
	return parameters;
}

bool FunctionString::parameterWasString( Uint32 index ) const {
	eeASSERT( index < parameters.size() );
	eeASSERT( index < typeStringData.size() );
	return typeStringData[index];
}

bool FunctionString::isEmpty() const {
	return name.empty();
}

}} // namespace EE::System
