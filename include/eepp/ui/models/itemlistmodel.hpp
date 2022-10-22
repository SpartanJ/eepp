#ifndef EE_UI_MODELS_ITEMLISTMODEL_HPP
#define EE_UI_MODELS_ITEMLISTMODEL_HPP

#include <eepp/ui/models/model.hpp>
#include <memory>
#include <vector>

namespace EE { namespace UI { namespace Models {

template <typename T> class ItemListModel final : public Model {
  public:
	static std::shared_ptr<ItemListModel> create( std::vector<T>& data ) {
		return std::make_shared( *new ItemListModel<T>( data ) );
	}

	virtual ~ItemListModel() {}

	virtual size_t rowCount( const ModelIndex& ) const { return mData.size(); }

	virtual size_t columnCount( const ModelIndex& ) const { return 1; }

	virtual std::string columnName( const size_t& ) const { return "Data"; }

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		if ( role == ModelRole::Display )
			return Variant( mData[index.row()] );
		return {};
	}

	virtual void update() { onModelUpdate(); }

  private:
	explicit ItemListModel( std::vector<T>& data ) : mData( data ) {}

	std::vector<T>& mData;
};

template <typename K, typename V> class ItemPairListModel final : public Model {
  public:
	static std::shared_ptr<ItemPairListModel> create( std::vector<std::pair<K, V>>& data ) {
		return std::make_shared( *new ItemPairListModel<K, V>( data ) );
	}

	virtual ~ItemPairListModel() {}

	virtual size_t rowCount( const ModelIndex& ) const { return mData.size(); }

	virtual size_t columnCount( const ModelIndex& ) const { return 2; }

	virtual std::string columnName( const size_t& index ) const {
		eeASSERT( index < 2 );
		return mColumnNames[index];
	}

	virtual void setColumnName( const size_t& index, const std::string& name ) {
		eeASSERT( index < 2 );
		mColumnNames[index] = name;
	}

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		if ( role == ModelRole::Display ) {
			switch ( index.column() ) {
				case 0:
					return Variant( mData[index.row()].first );
				case 1:
				default:
					return Variant( mData[index.row()].second );
			}
		}
		return {};
	}

	virtual void update() { onModelUpdate(); }

  private:
	explicit ItemPairListModel( std::vector<K, V>& data ) : mData( data ) {}

	std::vector<std::pair<K, V>>& mData;
	std::vector<std::string> mColumnNames{ "Title", "Description" };
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_ITEMLISTMODEL_HPP
