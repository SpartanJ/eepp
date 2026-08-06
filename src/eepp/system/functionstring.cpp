#include <eepp/core/debug.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/functionstring.hpp>

#include <string>

namespace EE { namespace System {

FunctionString FunctionString::parseOwned( std::string_view function ) {
	View view = parseView( function );
	if ( view.isEmpty() )
		return {};
	Parameters funcParameters;
	TypeStringVector typeStringData;
	view.forEachParameter( [&]( std::string_view parameter, bool wasString ) {
		std::string owned;
		owned.reserve( parameter.size() );
		const char quote = wasString ? function[parameter.data() - function.data() - 1] : 0;
		for ( std::size_t i = 0; i < parameter.size(); ++i ) {
			if ( wasString && parameter[i] == '\\' && i + 1 < parameter.size() &&
				 parameter[i + 1] == quote )
				continue;
			owned += parameter[i];
		}
		funcParameters.emplace_back( std::move( owned ) );
		typeStringData.emplace_back( wasString );
		return true;
	} );
	return FunctionString( std::string{ view.getName() }, std::move( funcParameters ),
						   std::move( typeStringData ) );
}

FunctionString::View FunctionString::parseView( std::string_view function ) {
	const std::size_t open = function.find( '(' );
	if ( open == std::string_view::npos )
		return {};
	const std::size_t close = function.rfind( ')' );
	if ( close == std::string_view::npos || close < open )
		return {};
	std::string_view name = String::trim( function.substr( 0, open ), " \t\n\r\f\v" );
	if ( name.empty() )
		return {};
	return View{ name, function.substr( open + 1, close - open - 1 ) };
}

FunctionString FunctionString::parse( const std::string& function ) {
	return parseOwned( function );
}

FunctionString FunctionString::parse( std::string_view function ) {
	return parseOwned( function );
}

FunctionString FunctionString::parse( String::View function ) {
	return parseOwned( String( function ).toUtf8() );
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
