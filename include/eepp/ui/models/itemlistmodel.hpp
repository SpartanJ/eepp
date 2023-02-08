#ifndef EE_UI_MODELS_ITEMLISTMODEL_HPP
#define EE_UI_MODELS_ITEMLISTMODEL_HPP

#include <eepp/ui/models/model.hpp>
#include <memory>
#include <vector>

namespace EE { namespace UI { namespace Models {

template <typename T> class ItemListModel final : public Model {
  public:
	static std::shared_ptr<ItemListModel> create( const std::vector<T>& data ) {
		return std::make_shared<ItemListModel<T>>( data );
	}

	explicit ItemListModel( const std::vector<T>& data ) : mData( data ) {}

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
	const std::vector<T>& mData;
};

template <typename K, typename V> class ItemPairListModel final : public Model {
  public:
	static std::shared_ptr<ItemPairListModel> create( const std::vector<std::pair<K, V>>& data ) {
		return std::make_shared<ItemPairListModel<K, V>>( data );
	}

	explicit ItemPairListModel( const std::vector<std::pair<K, V>>& data ) : mData( data ) {}

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
	const std::vector<std::pair<K, V>>& mData;
	std::vector<std::string> mColumnNames{ "Title", "Description" };
};

template <typename T> class ItemListOwnerModel final : public Model {
  public:
	static std::shared_ptr<ItemListOwnerModel> create( const std::vector<T>& data ) {
		return std::make_shared<ItemListOwnerModel<T>>( data );
	}

	explicit ItemListOwnerModel( const std::vector<T>& data ) : mData( data ) {}

	virtual ~ItemListOwnerModel() {}

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
	std::vector<T> mData;
};

template <typename K, typename V> class ItemPairListOwnerModel final : public Model {
  public:
	static std::shared_ptr<ItemPairListOwnerModel>
	create( const std::vector<std::pair<K, V>>& data ) {
		return std::make_shared<ItemPairListOwnerModel<K, V>>( data );
	}

	explicit ItemPairListOwnerModel( const std::vector<std::pair<K, V>>& data ) : mData( data ) {}

	virtual ~ItemPairListOwnerModel() {}

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
	std::vector<std::pair<K, V>> mData;
	std::vector<std::string> mColumnNames{ "Title", "Description" };
};

template <typename V> class ItemVectorListOwnerModel final : public Model {
  public:
	static std::shared_ptr<ItemVectorListOwnerModel>
	create( const size_t& columnCount, const std::vector<std::vector<V>>& data ) {
		return std::make_shared<ItemVectorListOwnerModel<V>>( columnCount, data );
	}

	explicit ItemVectorListOwnerModel( const size_t& columnCount,
									   const std::vector<std::vector<V>>& data ) :
		mData( data ) {
		mColumnNames.resize( columnCount );
	}

	virtual ~ItemVectorListOwnerModel() {}

	virtual size_t rowCount( const ModelIndex& ) const { return mData.size(); }

	virtual size_t columnCount( const ModelIndex& ) const { return mColumnNames.size(); }

	virtual std::string columnName( const size_t& index ) const {
		eeASSERT( index < mColumnNames.size() );
		return mColumnNames[index];
	}

	virtual void setColumnName( const size_t& index, const std::string& name ) {
		eeASSERT( index < mColumnNames.size() );
		mColumnNames[index] = name;
	}

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		eeASSERT( index.row() < (Int64)mData.size() );
		eeASSERT( index.column() < (Int64)mData[index.row()].size() );
		if ( index.row() >= (Int64)mData.size() ||
			 index.column() >= (Int64)mData[index.row()].size() )
			return {};

		if ( role == ModelRole::Display )
			return Variant( mData[index.row()][index.column()] );

		return {};
	}

	virtual void update() { onModelUpdate(); }

  private:
	std::vector<std::vector<V>> mData;
	std::vector<std::string> mColumnNames;
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_ITEMLISTMODEL_HPP
