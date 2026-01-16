#include <eepp/core/debug.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/functionstring.hpp>

namespace EE { namespace System {

FunctionString FunctionString::parse( const std::string& function ) {
	size_t funcSep = function.find( '(' );
	if ( funcSep == std::string::npos )
		return FunctionString( "", {}, {} );

	std::string funcName = function.substr( 0, funcSep );
	String::trimInPlace( funcName );

	std::vector<std::string> funcParameters;
	std::vector<bool> typeStringData;

	std::string parametersString = function.substr( funcSep + 1 );
	size_t paramClose = parametersString.find_last_of( ')' );
	if ( paramClose == std::string::npos )
		return FunctionString( "", {}, {} );

	parametersString = parametersString.substr( 0, paramClose );

	bool stateParsingString = false;
	std::string buffer = "";
	char prevChar = 0;
	bool currentParamIsString = false;
	int parenDepth = 0;

	for ( size_t i = 0; i < parametersString.length(); ++i ) {
		char c = parametersString[i];

		if ( !stateParsingString ) {
			if ( c == '(' ) {
				parenDepth++;
				buffer += c;
			} else if ( c == ')' ) {
				if ( parenDepth > 0 )
					parenDepth--;
				buffer += c;
			} else if ( c == ',' ) {
				if ( parenDepth == 0 ) {
					if ( !buffer.empty() ) {
						if ( !currentParamIsString )
							String::trimInPlace( buffer );
						funcParameters.push_back( buffer );
						typeStringData.push_back( currentParamIsString );
						buffer = "";
						currentParamIsString = false;
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

	if ( !buffer.empty() ) {
		if ( !currentParamIsString )
			String::trimInPlace( buffer );
		funcParameters.push_back( buffer );
		typeStringData.push_back( currentParamIsString );
	}

	return FunctionString( funcName, funcParameters, typeStringData );
}

FunctionString::FunctionString( const std::string& name, const std::vector<std::string>& parameters,
								const std::vector<bool>& typeStringData ) :
	name( name ), parameters( parameters ), typeStringData( typeStringData ) {}

const std::string& FunctionString::getName() const {
	return name;
}

const std::vector<std::string>& FunctionString::getParameters() const {
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
