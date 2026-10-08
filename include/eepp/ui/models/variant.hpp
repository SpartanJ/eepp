#ifndef EE_UI_MODEL_VARIANT_HPP
#define EE_UI_MODEL_VARIANT_HPP

#include <cstring>
#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/ui/uiicon.hpp>
#include <new>
#include <string>
#include <string_view>
#include <utility>

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
		StringPtr,
		StdStringPtr
	};

	Variant() = default;

	explicit Variant( const std::string& string ) {
		new ( &mValue.asStdString ) std::string( string );
		mType = Type::StdString;
	}

	explicit Variant( std::string&& string ) {
		new ( &mValue.asStdString ) std::string( std::move( string ) );
		mType = Type::StdString;
	}

	explicit Variant( const std::string* string ) : mType( Type::StdStringPtr ) {
		mValue.asStdStringPtr = string;
	}

	explicit Variant( const String& string ) {
		new ( &mValue.asString ) String( string );
		mType = Type::String;
	}

	explicit Variant( String&& string ) {
		new ( &mValue.asString ) String( std::move( string ) );
		mType = Type::String;
	}

	explicit Variant( const String* string ) : mType( Type::StringPtr ) {
		mValue.asStringPtr = string;
	}

	Variant( DrawablePtr drawable ) {
		new ( &mValue.asDrawable ) DrawablePtr( std::move( drawable ) );
		mType = Type::Drawable;
	}

	Variant( UIIcon* icon ) : mType( Type::Icon ) { mValue.asIcon = icon; }

	Variant( const Vector2f& v ) {
		new ( &mValue.asVector2f ) Vector2f( v );
		mType = Type::Vector2f;
	}

	Variant( void* data ) : mType( Type::DataPtr ) { mValue.asDataPtr = data; }

	Variant( const Rectf& r ) {
		new ( &mValue.asRectf ) Rectf( r );
		mType = Type::Rectf;
	}

	Variant( bool val ) : mType( Type::Bool ) { mValue.asBool = val; }

	Variant( const Float& val ) : mType( Type::Float ) { mValue.asFloat = val; }

	Variant( const int& val ) : mType( Type::Int ) { mValue.asInt = val; }

	Variant( const unsigned int& val ) : mType( Type::Uint ) { mValue.asUint = val; }

	Variant( const Int64& val ) : mType( Type::Int64 ) { mValue.asInt64 = val; }

	Variant( const Uint64& val ) : mType( Type::Uint64 ) { mValue.asUint64 = val; }

	explicit Variant( const char* data ) : mType( Type::cstr ) { mValue.asCStr = data; }

	~Variant() { destroy(); }

	Variant( const Variant& other ) { copyConstruct( other ); }

	Variant( Variant&& other ) { moveConstruct( std::move( other ) ); }

	Variant& operator=( const Variant& other ) {
		if ( this == &other )
			return *this;
		if ( mType == other.mType ) {
			copyAssignSameType( other );
		} else {
			destroy();
			copyConstruct( other );
		}
		return *this;
	}

	Variant& operator=( Variant&& other ) {
		if ( this == &other )
			return *this;
		if ( mType == other.mType ) {
			moveAssignSameType( std::move( other ) );
		} else {
			destroy();
			moveConstruct( std::move( other ) );
		}
		return *this;
	}

	static Variant fromRef( const std::string& string ) { return Variant( &string ); }

	const std::string& asStdString() const { return mValue.asStdString; }

	const std::string& asStdStringPtr() const { return *mValue.asStdStringPtr; }

	const String& asString() const { return mValue.asString; }

	const String& asStringPtr() const { return *mValue.asStringPtr; }

	const DrawablePtr& asDrawable() const { return mValue.asDrawable; }

	const bool& asBool() const { return mValue.asBool; }

	const Float& asFloat() const { return mValue.asFloat; }

	const int& asInt() const { return mValue.asInt; }

	const unsigned int& asUint() const { return mValue.asUint; }

	const Int64& asInt64() const { return mValue.asInt64; }

	const Uint64& asUint64() const { return mValue.asUint64; }

	const Vector2f& asVector2f() const { return mValue.asVector2f; }

	const Rectf& asRectf() const { return mValue.asRectf; }

	const char* asCStr() const { return mValue.asCStr; }

	UIIcon* asIcon() const { return mValue.asIcon; }

	void* asDataPtr() const { return mValue.asDataPtr; }

	Type getType() const { return mType; }

	bool is( const Type& type ) const { return type == mType; }

	bool isString() const {
		return mType == Type::StdString || mType == Type::StdStringPtr || mType == Type::cstr ||
			   mType == Type::String || mType == Type::StringPtr;
	}

	bool isStdStringLike() const {
		return mType == Type::StdString || mType == Type::StdStringPtr || mType == Type::cstr;
	}

	std::string_view asStdStringView() const {
		switch ( mType ) {
			case Type::StdString:
				return asStdString();
			case Type::StdStringPtr:
				return asStdStringPtr();
			case Type::cstr:
				return asCStr();
			default:
				return {};
		}
	}

	void reset() { destroy(); }

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
			case Type::StdStringPtr:
				return asStdStringPtr();
			case Type::String:
				return asString();
			case Type::StringPtr:
				return asStringPtr();
			case Type::Drawable:
				return asDrawable()->isDrawableResource()
						   ? static_cast<DrawableResource*>( asDrawable().get() )->getName()
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
		if ( mType != other.mType ) {
			if ( isStdStringLike() && other.isStdStringLike() )
				return asStdStringView() < other.asStdStringView();
			return toString() < other.toString();
		}
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
			case Type::StdStringPtr:
				return asStdStringPtr() < other.asStdStringPtr();
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
				return std::strcmp( asCStr(), other.asCStr() ) < 0;
			case Type::Invalid:
				break;
		}
		return false;
	}

	bool operator==( const Variant& other ) const {
		if ( mType != other.mType ) {
			if ( isStdStringLike() && other.isStdStringLike() )
				return asStdStringView() == other.asStdStringView();
			return toString() == other.toString();
		}
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
			case Type::StdStringPtr:
				return asStdStringPtr() == other.asStdStringPtr();
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
				return std::strcmp( asCStr(), other.asCStr() ) == 0;
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
			case Type::StdStringPtr:
				return asStdStringPtr().size();
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
				return std::strlen( asCStr() );
			case Type::Invalid:
				break;
		}
		return 0;
	}

  private:
	union Storage {
		void* asDataPtr;
		UIIcon* asIcon;
		std::string asStdString;
		const std::string* asStdStringPtr;
		String asString;
		const String* asStringPtr;
		DrawablePtr asDrawable;
		bool asBool;
		Float asFloat;
		int asInt;
		unsigned int asUint;
		Int64 asInt64;
		Uint64 asUint64;
		Vector2f asVector2f;
		Rectf asRectf;
		const char* asCStr;

		Storage() {}
		~Storage() {}
	} mValue;

	Type mType{ Type::Invalid };

	void destroy() {
		switch ( mType ) {
			case Type::StdString:
				mValue.asStdString.~basic_string();
				break;
			case Type::String:
				mValue.asString.~String();
				break;
			case Type::Drawable:
				mValue.asDrawable.~DrawablePtr();
				break;
			default:
				break;
		}
		mType = Type::Invalid;
	}

	void copyConstruct( const Variant& other ) {
		switch ( other.mType ) {
			case Type::StdString:
				new ( &mValue.asStdString ) std::string( other.mValue.asStdString );
				break;
			case Type::StdStringPtr:
				mValue.asStdStringPtr = other.mValue.asStdStringPtr;
				break;
			case Type::String:
				new ( &mValue.asString ) String( other.mValue.asString );
				break;
			case Type::StringPtr:
				mValue.asStringPtr = other.mValue.asStringPtr;
				break;
			case Type::Drawable:
				new ( &mValue.asDrawable ) DrawablePtr( other.mValue.asDrawable );
				break;
			case Type::Icon:
				mValue.asIcon = other.mValue.asIcon;
				break;
			case Type::Vector2f:
				new ( &mValue.asVector2f ) Vector2f( other.mValue.asVector2f );
				break;
			case Type::Rectf:
				new ( &mValue.asRectf ) Rectf( other.mValue.asRectf );
				break;
			case Type::Bool:
				mValue.asBool = other.mValue.asBool;
				break;
			case Type::Float:
				mValue.asFloat = other.mValue.asFloat;
				break;
			case Type::Int:
				mValue.asInt = other.mValue.asInt;
				break;
			case Type::Uint:
				mValue.asUint = other.mValue.asUint;
				break;
			case Type::Int64:
				mValue.asInt64 = other.mValue.asInt64;
				break;
			case Type::Uint64:
				mValue.asUint64 = other.mValue.asUint64;
				break;
			case Type::cstr:
				mValue.asCStr = other.mValue.asCStr;
				break;
			case Type::DataPtr:
				mValue.asDataPtr = other.mValue.asDataPtr;
				break;
			case Type::Invalid:
				break;
		}
		mType = other.mType;
	}

	void moveConstruct( Variant&& other ) {
		switch ( other.mType ) {
			case Type::StdString:
				new ( &mValue.asStdString ) std::string( std::move( other.mValue.asStdString ) );
				break;
			case Type::StdStringPtr:
				mValue.asStdStringPtr = other.mValue.asStdStringPtr;
				break;
			case Type::String:
				new ( &mValue.asString ) String( std::move( other.mValue.asString ) );
				break;
			case Type::StringPtr:
				mValue.asStringPtr = other.mValue.asStringPtr;
				break;
			case Type::Drawable:
				new ( &mValue.asDrawable ) DrawablePtr( std::move( other.mValue.asDrawable ) );
				break;
			case Type::Icon:
				mValue.asIcon = other.mValue.asIcon;
				break;
			case Type::Vector2f:
				new ( &mValue.asVector2f ) Vector2f( std::move( other.mValue.asVector2f ) );
				break;
			case Type::Rectf:
				new ( &mValue.asRectf ) Rectf( std::move( other.mValue.asRectf ) );
				break;
			case Type::Bool:
				mValue.asBool = other.mValue.asBool;
				break;
			case Type::Float:
				mValue.asFloat = other.mValue.asFloat;
				break;
			case Type::Int:
				mValue.asInt = other.mValue.asInt;
				break;
			case Type::Uint:
				mValue.asUint = other.mValue.asUint;
				break;
			case Type::Int64:
				mValue.asInt64 = other.mValue.asInt64;
				break;
			case Type::Uint64:
				mValue.asUint64 = other.mValue.asUint64;
				break;
			case Type::cstr:
				mValue.asCStr = other.mValue.asCStr;
				break;
			case Type::DataPtr:
				mValue.asDataPtr = other.mValue.asDataPtr;
				break;
			case Type::Invalid:
				break;
		}
		mType = other.mType;
		other.destroy();
	}

	void copyAssignSameType( const Variant& other ) {
		switch ( mType ) {
			case Type::StdString:
				mValue.asStdString = other.mValue.asStdString;
				break;
			case Type::String:
				mValue.asString = other.mValue.asString;
				break;
			case Type::Drawable:
				mValue.asDrawable = other.mValue.asDrawable;
				break;
			default:
				destroy();
				copyConstruct( other );
				break;
		}
	}

	void moveAssignSameType( Variant&& other ) {
		switch ( mType ) {
			case Type::StdString:
				mValue.asStdString = std::move( other.mValue.asStdString );
				other.destroy();
				break;
			case Type::String:
				mValue.asString = std::move( other.mValue.asString );
				other.destroy();
				break;
			case Type::Drawable:
				mValue.asDrawable = std::move( other.mValue.asDrawable );
				other.destroy();
				break;
			default:
				destroy();
				moveConstruct( std::move( other ) );
				break;
		}
	}
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODEL_VARIANT_HPP
