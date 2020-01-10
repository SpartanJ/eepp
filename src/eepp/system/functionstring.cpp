#include <eepp/core/string.hpp>
#include <eepp/system/functionstring.hpp>

namespace EE { namespace System {

FunctionString FunctionString::parse( const std::string& function ) {
	size_t posFuncStart = function.find_first_of( '(' );
	size_t posFuncEnd = function.find_last_of( ')' );
	std::string funcName;
	std::vector<std::string> parameters;

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
							curParameter = "";
						}
					} else if ( '"' == curChar ) {
						parsingString = true;
					} else {
						curParameter += curChar;
					}
				} else {
					if ( '"' == curChar && !lastWasBackslash ) {
						parsingString = false;

						if ( !curParameter.empty() ) {
							parameters.push_back( curParameter );
							curParameter = "";
						}
					} else {
						if ( '\\' == curChar )
							lastWasBackslash = !lastWasBackslash;

						curParameter += curChar;
					}
				}
			}

			curParameter = String::trim( curParameter );

			if ( !curParameter.empty() )
				parameters.push_back( curParameter );
		}
	}

	return FunctionString( funcName, parameters );
}

FunctionString::FunctionString( const std::string& name,
								const std::vector<std::string>& parameters ) :
	name( name ), parameters( parameters ) {}

const std::string& FunctionString::getName() const {
	return name;
}

const std::vector<std::string>& FunctionString::getParameters() const {
	return parameters;
}

bool FunctionString::isEmpty() const {
	return name.empty();
}

}} // namespace EE::System
