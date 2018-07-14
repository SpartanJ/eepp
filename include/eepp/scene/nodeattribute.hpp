#ifndef EE_NODEATTRIBUTE_HPP
#define EE_NODEATTRIBUTE_HPP

#include <string>
#include <eepp/core/string.hpp>

namespace EE { namespace Scene {

class NodeAttribute {
	public:
		NodeAttribute( std::string name, std::string value );

		std::string getName() const;

		void setName(const std::string & name);

		const std::string& getValue() const;

		const std::string& value() const;

		void setValue(const std::string & value);

		std::string asString( const std::string& defaultValue = "" ) const;

		template<typename Type>
		Type asType( Type defaultValue ) const {
			Type val = defaultValue;
			return String::fromString<Type>( val, mValue ) ? val : defaultValue;
		}

		int asInt( int defaultValue = 0 ) const;

		unsigned int asUint( unsigned int defaultValue = 0 ) const;

		double asDouble( double defaultValue = 0 ) const;

		float asFloat( float defaultValue = 0 ) const;

		long long asLlong( long long defaultValue = 0)  const;

		unsigned long long asUllong( unsigned long long defaultValue = 0 ) const;

		bool asBool( bool defaultValue = false ) const;

	protected:
		std::string mName;
		std::string mValue;
};

}}

#endif
