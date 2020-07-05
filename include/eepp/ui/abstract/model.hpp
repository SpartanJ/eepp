#ifndef EE_UI_MODEL_MODEL_HPP
#define EE_UI_MODEL_MODEL_HPP

#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/ui/abstract/modelindex.hpp>
#include <functional>
#include <string>
#include <unordered_set>

using namespace EE::Graphics;
using namespace EE::Math;

namespace EE { namespace UI { namespace Abstract {

enum class SortOrder { None, Ascending, Descending };

class UIAbstractView;

class Variant {
  public:
	enum class Type {
		Invalid,
		DataPtr,
		String,
		Bool,
		Float,
		Int,
		Int64,
		Drawable,
		Vector2f,
		Rectf
	};
	Variant() : mType( Type::Invalid ) {}
	Variant( const std::string& string ) : mType( Type::String ) {
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
	Variant( const Int64& val ) : mType( Type::Int64 ) { mValue.asInt64 = val; }
	~Variant() { reset(); }
	const std::string& asString() const { return *mValue.asString; }
	const Drawable* asDrawable() const { return mValue.asDrawable; }
	const bool& asBool() const { return mValue.asBool; }
	const Float& asFloat() const { return mValue.asFloat; }
	const int& asInt() const { return mValue.asInt; }
	const Int64& asInt64() const { return mValue.asInt64; }
	const Vector2f& asVector2f() const { return *mValue.asVector2f; }
	const Rectf& asRectf() const { return *mValue.asRectf; }
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
		Int64 asInt64;
		Vector2f* asVector2f;
		Rectf* asRectf;
	} mValue;
	Type mType;
	bool mOwnsObject{false};
};

class EE_API Model {
  public:
	enum UpdateFlag {
		DontInvalidateIndexes = 0,
		InvalidateAllIndexes = 1 << 0,
	};

	enum class Role {
		Display,
		Icon,
	};

	virtual ~Model(){};

	virtual int rowCount( const ModelIndex& = ModelIndex() ) const = 0;

	virtual int columnCount( const ModelIndex& = ModelIndex() ) const = 0;

	virtual std::string columnName( const size_t& /*column*/ ) const { return {}; }

	virtual Variant data( const ModelIndex&, Role = Role::Display ) const = 0;

	virtual void update() = 0;

	virtual ModelIndex parentIndex( const ModelIndex& ) const { return {}; }

	virtual ModelIndex index( int row, int column = 0, const ModelIndex& = ModelIndex() ) const {
		return createIndex( row, column );
	}

	virtual ModelIndex sibling( int row, int column, const ModelIndex& parent ) const;

	virtual void setData( const ModelIndex&, const Variant& ) {}

	virtual size_t treeColumn() const { return 0; }

	virtual bool acceptsDrag( const ModelIndex&, const std::string& dataType );

	virtual bool isColumnSortable( const size_t& /*columnIndex*/ ) const { return true; }

	virtual std::string dragDataType() const { return {}; }

	bool isValid( const ModelIndex& index ) const {
		auto parentIndex = this->parentIndex( index );
		return index.row() >= 0 && index.row() < rowCount( parentIndex ) && index.column() >= 0 &&
			   index.column() < columnCount( parentIndex );
	}

	virtual int keyColumn() const { return -1; }

	virtual SortOrder sortOrder() const { return SortOrder::None; }

	virtual void setKeyColumnAndSortOrder( const size_t& /*column*/, const SortOrder& /*order*/ ) {}

	void registerView( UIAbstractView* );

	void unregisterView( UIAbstractView* );

	void setOnUpdate( const std::function<void()>& onUpdate );

  protected:
	Model(){};

	void forEachView( std::function<void( UIAbstractView* )> );

	void onModelUpdate( unsigned flags = UpdateFlag::InvalidateAllIndexes );

	ModelIndex createIndex( int row, int column, const void* data = nullptr ) const;

  private:
	std::unordered_set<UIAbstractView*> mViews;
	std::function<void()> mOnUpdate;
};

inline ModelIndex ModelIndex::parent() const {
	return mModel ? mModel->parentIndex( *this ) : ModelIndex();
}

}}} // namespace EE::UI::Abstract

#endif // EE_UI_MODEL_MODEL_HPP
