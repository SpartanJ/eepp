#ifndef EE_UI_MODEL_VARIANT_HPP
#define EE_UI_MODEL_VARIANT_HPP

#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/ui/uiicon.hpp>
#include <string>

using namespace EE::Graphics;
using namespace EE::Math;

namespace EE { namespace UI { namespace Models {

class EE_API Variant {
  public:
	enum class Type {
		Invalid,
		DataPtr,
		String,
		StdString,
		Bool,
		Float,
		Int,
		Uint,
		Int64,
		Uint64,
		Drawable,
		Icon,
		Vector2f,
		Rectf,
		cstr,
		StringPtr
	};
	Variant() : mType( Type::Invalid ) {}
	explicit Variant( const std::string& string ) : mType( Type::StdString ) {
		mValue.asStdString = eeNew( std::string, ( string ) );
	}
	explicit Variant( const String& string ) : mType( Type::String ) {
		mValue.asString = eeNew( String, ( string ) );
	}
	explicit Variant( const String* string ) : mType( Type::StringPtr ) { mValue.asStringPtr = string; }
	Variant( Drawable* drawable, bool ownDrawable = false ) : mType( Type::Drawable ) {
		mValue.asDrawable = drawable;
		mOwnsObject = ownDrawable;
	}
	Variant( UIIcon* icon ) : mType( Type::Icon ) { mValue.asIcon = icon; }
	Variant( const Vector2f& v ) : mType( Type::Vector2f ) {
		mValue.asVector2f = eeNew( Vector2f, ( v ) );
	}
	Variant( void* data ) : mType( Type::DataPtr ) { mValue.asDataPtr = data; }
	Variant( const Rectf& r ) : mType( Type::Rectf ) { mValue.asRectf = eeNew( Rectf, ( r ) ); }
	Variant( bool val ) : mType( Type::Bool ) { mValue.asBool = val; }
	Variant( const Float& val ) : mType( Type::Float ) { mValue.asFloat = val; }
	Variant( const int& val ) : mType( Type::Int ) { mValue.asInt = val; }
	Variant( const unsigned int& val ) : mType( Type::Int ) { mValue.asUint = val; }
	Variant( const Int64& val ) : mType( Type::Int64 ) { mValue.asInt64 = val; }
	Variant( const Uint64& val ) : mType( Type::Uint64 ) { mValue.asUint64 = val; }
	explicit Variant( const char* data ) : mType( Type::cstr ) { mValue.asCStr = data; }
	~Variant() { reset(); }
	const std::string& asStdString() const { return *mValue.asStdString; }
	const String& asString() const { return *mValue.asString; }
	const String& asStringPtr() const { return *mValue.asStringPtr; }
	Drawable* asDrawable() const { return mValue.asDrawable; }
	const bool& asBool() const { return mValue.asBool; }
	const Float& asFloat() const { return mValue.asFloat; }
	const int& asInt() const { return mValue.asInt; }
	const unsigned int& asUint() const { return mValue.asUint; }
	const Int64& asInt64() const { return mValue.asInt64; }
	const Uint64& asUint64() const { return mValue.asUint64; }
	const Vector2f& asVector2f() const { return *mValue.asVector2f; }
	const Rectf& asRectf() const { return *mValue.asRectf; }
	const char* asCStr() const { return mValue.asCStr; }
	UIIcon* asIcon() const { return mValue.asIcon; }
	void* asDataPtr() const { return mValue.asDataPtr; }
	bool is( const Type& type ) const { return type == mType; }
	bool isString() const {
		return mType == Type::StdString || mType == Type::cstr || mType == Type::String ||
			   mType == Type::StringPtr;
	}
	void reset() {
		switch ( mType ) {
			case Type::StdString:
				eeSAFE_DELETE( mValue.asStdString );
				break;
			case Type::String:
				eeSAFE_DELETE( mValue.asString );
				break;
			case Type::Drawable:
				if ( mOwnsObject )
					eeSAFE_DELETE( mValue.asDrawable );
				break;
			case Type::Vector2f:
				eeSAFE_DELETE( mValue.asVector2f );
				break;
			case Type::Rectf:
				eeSAFE_DELETE( mValue.asRectf );
				break;
			default:
				break;
		}
		mType = Type::Invalid;
	}
	bool isValid() const { return mType != Type::Invalid; }

	std::string toString() const {
		switch ( mType ) {
			case Type::Bool:
				return asBool() ? "true" : "false";
			case Type::Int:
				return String::toString( asInt() );
			case Type::Uint:
				return String::toString( asUint() );
			case Type::Int64:
				return String::toString( asInt64() );
			case Type::Uint64:
				return String::toString( asUint64() );
			case Type::Float:
				return String::toString( asFloat() );
			case Type::StdString:
				return asStdString();
			case Type::String:
				return asString();
			case Type::StringPtr:
				return asStringPtr();
			case Type::Drawable:
				return asDrawable()->isDrawableResource()
						   ? static_cast<DrawableResource*>( asDrawable() )->getName()
						   : "Drawable";
			case Type::Icon:
				return asIcon()->getName();
			case Type::DataPtr:
				return String::format( "%p", asDataPtr() );
			case Type::Vector2f:
				return String::format( "%.2f-%.2f", asVector2f().x, asVector2f().y );
			case Type::Rectf:
				return String::format( "%.2f-%.2f-%.2f-%.2f", asRectf().Top, asRectf().Right,
									   asRectf().Bottom, asRectf().Left );
			case Type::cstr:
				return asCStr();
			case Type::Invalid:
				break;
		}
		return "";
	}

	bool operator<( const Variant& other ) const {
		if ( mType != other.mType )
			return toString() < other.toString();
		switch ( mType ) {
			case Type::Bool:
				return asBool() < other.asBool();
			case Type::Int:
				return asInt() < other.asInt();
			case Type::Uint:
				return asUint() < other.asUint();
			case Type::Int64:
				return asInt64() < other.asInt64();
			case Type::Uint64:
				return asUint64() < other.asUint64();
			case Type::Float:
				return asFloat() < other.asFloat();
			case Type::StdString:
				return asStdString() < other.asStdString();
			case Type::String:
				return asString() < other.asString();
			case Type::StringPtr:
				return asStringPtr() < other.asStringPtr();
			case Type::Drawable:
				return asDrawable() < other.asDrawable();
			case Type::Icon:
				return asIcon() < other.asIcon();
			case Type::DataPtr:
				return asDataPtr() < other.asDataPtr();
			case Type::Vector2f:
				return asVector2f() < other.asVector2f();
			case Type::Rectf:
				return asRectf().getSize() < other.asRectf().getSize();
			case Type::cstr:
				return strcmp( asCStr(), other.asCStr() ) < 0;
			case Type::Invalid:
				break;
		}
		return false;
	}

	bool operator==( const Variant& other ) const {
		if ( mType != other.mType )
			return toString() == other.toString();
		switch ( mType ) {
			case Type::Bool:
				return asBool() == other.asBool();
			case Type::Int:
				return asInt() == other.asInt();
			case Type::Uint:
				return asUint() == other.asUint();
			case Type::Int64:
				return asInt64() == other.asInt64();
			case Type::Uint64:
				return asUint64() == other.asUint64();
			case Type::Float:
				return asFloat() == other.asFloat();
			case Type::StdString:
				return asStdString() == other.asStdString();
			case Type::String:
				return asString() == other.asString();
			case Type::StringPtr:
				return asStringPtr() == other.asStringPtr();
			case Type::Drawable:
				return asDrawable() == other.asDrawable();
			case Type::Icon:
				return asIcon() == other.asIcon();
			case Type::DataPtr:
				return asDataPtr() == other.asDataPtr();
			case Type::Vector2f:
				return asVector2f() == other.asVector2f();
			case Type::Rectf:
				return asRectf().getSize() == other.asRectf().getSize();
			case Type::cstr:
				return strcmp( asCStr(), other.asCStr() ) == 0;
			case Type::Invalid:
				break;
		}
		return false;
	}

	size_t size() const {
		switch ( mType ) {
			case Type::Bool:
				return 1;
			case Type::Int:
				return sizeof( int );
			case Type::Uint:
				return sizeof( unsigned int );
			case Type::Int64:
				return sizeof( Int64 );
			case Type::Uint64:
				return sizeof( Uint64 );
			case Type::Float:
				return sizeof( Float );
			case Type::StdString:
				return asStdString().size();
			case Type::String:
				return asString().size();
			case Type::StringPtr:
				return asStringPtr().size();
			case Type::Drawable:
				return sizeof( mValue.asDrawable );
			case Type::Icon:
				return asIcon()->getName().size();
			case Type::DataPtr:
				return sizeof( mValue.asDataPtr );
			case Type::Vector2f:
				return sizeof( mValue.asVector2f );
			case Type::Rectf:
				return sizeof( mValue.asRectf );
			case Type::cstr:
				return strlen( asCStr() );
			case Type::Invalid:
				break;
		}
		return 0;
	}

  private:
	union {
		void* asDataPtr{ nullptr };
		Drawable* asDrawable;
		UIIcon* asIcon;
		std::string* asStdString;
		String* asString;
		const String* asStringPtr;
		bool asBool;
		Float asFloat;
		int asInt;
		unsigned int asUint;
		Int64 asInt64;
		Uint64 asUint64;
		Vector2f* asVector2f;
		Rectf* asRectf;
		const char* asCStr;
	} mValue;
	Type mType;
	bool mOwnsObject{ false };
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODEL_VARIANT_HPP
