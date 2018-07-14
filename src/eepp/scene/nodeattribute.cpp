#include <eepp/scene/nodeattribute.hpp>

namespace EE { namespace Scene {

NodeAttribute::NodeAttribute( std::string name, std::string value ) :
	mName( String::toLower( name ) ),
	mValue( value )
{}

std::string NodeAttribute::getName() const {
	return mName;
}

void NodeAttribute::setName( const std::string& name ) {
	mName = name;
}

const std::string& NodeAttribute::getValue() const {
	return mValue;
}

const std::string& NodeAttribute::value() const {
	return mValue;
}

void NodeAttribute::setValue( const std::string& value ) {
	mValue = value;
}

std::string NodeAttribute::asString( const std::string& defaultValue ) const {
	return mValue.empty() ? defaultValue : mValue;
}

int NodeAttribute::asInt( int defaultValue ) const {
	return asType<int>( defaultValue );
}

unsigned int NodeAttribute::asUint( unsigned int defaultValue ) const {
	return asType<unsigned int>( defaultValue );
}

double NodeAttribute::asDouble( double defaultValue ) const {
	return asType<double>( defaultValue );
}

float NodeAttribute::asFloat(float defaultValue) const {
	return asType<float>( defaultValue );
}

long long NodeAttribute::asLlong(long long defaultValue) const {
	return asType<long long>( defaultValue );
}

unsigned long long NodeAttribute::asUllong(unsigned long long defaultValue) const {
	return asType<unsigned long long>( defaultValue );
}

bool NodeAttribute::asBool( bool defaultValue ) const {
	if ( mValue.empty() )
		return defaultValue;

	// only look at first char
	char first = mValue[0];

	// 1*, t* (true), T* (True), y* (yes), Y* (YES)
	return (first == '1' || first == 't' || first == 'T' || first == 'y' || first == 'Y');
}

}}
