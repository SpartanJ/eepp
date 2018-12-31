#ifndef EE_NODEATTRIBUTE_HPP
#define EE_NODEATTRIBUTE_HPP

#include <string>
#include <eepp/math/rect.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/color.hpp>
#include <eepp/math/originpoint.hpp>
#include <eepp/graphics/blendmode.hpp>

using namespace EE::System;
using namespace EE::Math;
using namespace EE::Graphics;

namespace EE { namespace Scene {

class NodeAttribute {
	public:
		enum AttributeType
		{
			TypeString,
			TypeInt,
			TypeUint,
			TypeDouble,
			TypeFloat,
			TypeLongLong,
			TypeULongLong,
			TypeBoolean,
			TypeDimension,
			TypeDimensionInt,
			TypeOriginPoint,
			TypeBlendMode,
			TypeVector,
			TypeRect,
			TypeRectf
		};

		class Info
		{
			public:
				Info( AttributeType type, const std::string& name );

				Info( AttributeType type, const std::vector<std::string>& names );

				bool isName( const std::string& name );

				const AttributeType& getType() const;

				const std::vector<std::string>& getNames() const;
			protected:
				AttributeType type;

				std::vector<std::string> names;
		};

		NodeAttribute();

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

		Color asColor() const;

		Float asDpDimension( const std::string& defaultValue = "" ) const;

		int asDpDimensionI( const std::string& defaultValue = "" ) const;

		Uint32 asDpDimensionUint( const std::string& defaultValue = "" ) const;

		OriginPoint asOriginPoint() const;

		BlendMode asBlendMode() const;

		Vector2f asVector2f( const Vector2f& defaultValue = Vector2f::Zero ) const;

		Vector2i asVector2i( const Vector2i& defaultValue = Vector2i::Zero ) const;

		Sizef asSizef( const Sizef& defaultValue = Sizef::Zero ) const;

		Sizei asSizei( const Sizei& defaultValue = Sizei::Zero ) const;

		Rect asRect( const Rect& defaultValue = Rect() ) const;

		Rectf asRectf( const Rectf& defaultValue = Rectf() ) const;
	protected:
		std::string mName;
		std::string mValue;
};

}}

#endif
