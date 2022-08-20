#include <eepp/core/debug.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/functionstring.hpp>

namespace EE { namespace System {

FunctionString FunctionString::parse( const std::string& function ) {
	size_t posFuncStart = function.find_first_of( '(' );
	if ( posFuncStart == std::string::npos )
		return FunctionString( "", {}, {} );
	size_t posFuncEnd = function.find_last_of( ')' );
	std::string funcName;
	std::vector<std::string> parameters;
	std::vector<bool> typeStringData;

	if ( std::string::npos != posFuncStart && std::string::npos != posFuncEnd && posFuncStart > 1 &&
		 posFuncStart + 1 < function.size() ) {
		funcName = function.substr( 0, posFuncStart );

		if ( !funcName.empty() ) {
			std::string funcParameters =
				function.substr( posFuncStart + 1, posFuncEnd - posFuncStart - 1 );
			std::string curParameter = "";
			bool parsingString = false;
			bool lastWasBackslash = false;

			funcParameters = String::trim( funcParameters );

			for ( size_t i = 0; i < funcParameters.size(); i++ ) {
				const char& curChar = funcParameters.at( i );

				if ( !parsingString ) {
					if ( ',' == curChar ) {
						curParameter = String::trim( curParameter );

						if ( !curParameter.empty() ) {
							parameters.push_back( curParameter );
							typeStringData.push_back( false );
							curParameter = "";
						}
					} else if ( '"' == curChar ) {
						parsingString = true;
						curParameter = "";
					} else {
						curParameter += curChar;
					}
				} else {
					if ( '"' == curChar && !lastWasBackslash ) {
						parsingString = false;

						if ( !curParameter.empty() ) {
							parameters.push_back( curParameter );
							typeStringData.push_back( true );
							curParameter = "";
						}
					} else if ( '\\' != curChar || lastWasBackslash ) {
						curParameter += curChar;
					}
				}

				lastWasBackslash = '\\' == curChar;
			}

			curParameter = String::trim( curParameter );

			if ( !curParameter.empty() ) {
				parameters.push_back( curParameter );
				typeStringData.push_back( false );
			}
		}
	}

	return FunctionString( funcName, parameters, typeStringData );
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

bool FunctionString::parameterWasString( const Uint32& index ) const {
	eeASSERT( index < parameters.size() );
	eeASSERT( index < typeStringData.size() );
	return typeStringData[index];
}

bool FunctionString::isEmpty() const {
	return name.empty();
}

}} // namespace EE::System
