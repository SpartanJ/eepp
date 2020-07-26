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

	virtual Variant data( const ModelIndex& index, Role role = Role::Display ) const {
		if ( role == Role::Display )
			return Variant( mData[ index.row() ] );
		return {};
	}

	virtual void update() { onModelUpdate(); }

  private:
	explicit ItemListModel( std::vector<T>& data ) : mData( data ) {}

	std::vector<T>& mData;
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_ITEMLISTMODEL_HPP
