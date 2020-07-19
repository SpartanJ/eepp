#ifndef EE_UI_MODEL_VARIANT_HPP
#define EE_UI_MODEL_VARIANT_HPP

#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/math/rect.hpp>
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
		Bool,
		Float,
		Int,
		Uint,
		Int64,
		Uint64,
		Drawable,
		Vector2f,
		Rectf,
		cstr
	};
	Variant() : mType( Type::Invalid ) {}
	explicit Variant( const std::string& string ) : mType( Type::String ) {
		mValue.asString = eeNew( std::string, ( string ) );
	}
	Variant( Drawable* drawable, bool ownDrawable = false ) : mType( Type::Drawable ) {
		mValue.asDrawable = drawable;
		mOwnsObject = ownDrawable;
	}
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
	const std::string& asString() const { return *mValue.asString; }
	Drawable* asDrawable() const { return mValue.asDrawable; }
	const bool& asBool() const { return mValue.asBool; }
	const Float& asFloat() const { return mValue.asFloat; }
	const int& asInt() const { return mValue.asInt; }
	const Int64& asInt64() const { return mValue.asInt64; }
	const Uint64& asUint64() const { return mValue.asUint64; }
	const Vector2f& asVector2f() const { return *mValue.asVector2f; }
	const Rectf& asRectf() const { return *mValue.asRectf; }
	const char* asCStr() const { return mValue.asCStr; }
	bool is( const Type& type ) const { return type == mType; }
	void reset() {
		switch ( mType ) {
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
	bool isValid() { return mType != Type::Invalid; }

  private:
	union {
		void* asDataPtr;
		Drawable* asDrawable;
		std::string* asString;
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
	bool mOwnsObject{false};
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODEL_VARIANT_HPP
