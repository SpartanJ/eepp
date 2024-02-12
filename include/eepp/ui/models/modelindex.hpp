#ifndef EE_UI_MODEL_MODELINDEX_HPP
#define EE_UI_MODEL_MODELINDEX_HPP

#include <eepp/config.hpp>
#include <eepp/core/core.hpp>
#include <eepp/ui/models/modelrole.hpp>
#include <eepp/ui/models/variant.hpp>

namespace EE { namespace UI { namespace Models {

class Model;

class EE_API ModelIndex {
  public:
	ModelIndex() {}

	bool isValid() const { return mRow != -1 && mColumn != -1; }

	const Int64& row() const { return mRow; }

	const Int64& column() const { return mColumn; }

	Variant data( ModelRole = ModelRole::Display ) const;

	void* internalData() const { return mData; }

	template <typename T> T* ref() const { return static_cast<T*>( mData ); }

	Int64 internalId() const { return mInternalId; }

	ModelIndex parent() const;

	bool hasParent() const { return parent().isValid(); }

	bool operator<( const ModelIndex& other ) const {
		return mRow != other.mRow || mColumn != other.mColumn;
	}

	bool operator==( const ModelIndex& other ) const {
		return mModel == other.mModel && mRow == other.mRow && mColumn == other.mColumn &&
			   mData == other.mData;
	}

	bool operator!=( const ModelIndex& other ) const { return !( *this == other ); }

	const Model* model() const { return mModel; }

	ModelIndex sibling( int row, int column ) const;

	ModelIndex siblingAtColumn( int column ) const;

	std::string toString() const;

  protected:
	friend class Model;
	const Model* mModel{ nullptr };
	Int64 mRow{ -1 };
	Int64 mColumn{ -1 };
	void* mData{ nullptr };
	Int64 mInternalId{ 0 };

	ModelIndex( const Model& model, int row, int column, void* internalData,
				const Int64& internalId = 0 ) :
		mModel( &model ),
		mRow( row ),
		mColumn( column ),
		mData( internalData ),
		mInternalId( internalId ) {}
};

}}} // namespace EE::UI::Models

template <> struct std::hash<EE::UI::Models::ModelIndex> {
	std::size_t operator()( EE::UI::Models::ModelIndex const& modelIndex ) const noexcept {
		return hashCombine( reinterpret_cast<std::size_t>( modelIndex.model() ), modelIndex.row(),
							modelIndex.column(),
							reinterpret_cast<std::size_t>( modelIndex.internalData() ),
							modelIndex.internalId() );
	}
};

#endif // EE_UI_MODEL_MODELINDEX_HPP
