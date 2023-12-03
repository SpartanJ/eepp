#ifndef EE_UI_MODELS_SORTINGPROXYMODEL_HPP
#define EE_UI_MODELS_SORTINGPROXYMODEL_HPP

#include <eepp/ui/models/model.hpp>
#include <memory>

namespace EE { namespace UI { namespace Models {

class EE_API SortingProxyModel final : public Model, private Model::Client {
  public:
	template <typename T>
	static std::shared_ptr<SortingProxyModel> New( std::shared_ptr<T> model ) {
		return std::shared_ptr<SortingProxyModel>(
			new SortingProxyModel( std::static_pointer_cast<Model>( model ) ) );
	}

	virtual ~SortingProxyModel();

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const;

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const;

	virtual std::string columnName( const size_t& ) const;

	virtual Variant data( const ModelIndex& proxyIndex, ModelRole role = ModelRole::Display ) const;

	virtual ModelIndex parentIndex( const ModelIndex& ) const;

	virtual ModelIndex index( int row, int column = 0, const ModelIndex& = ModelIndex() ) const;

	virtual void update();

	virtual int keyColumn() const;

	virtual size_t treeColumn() const;

	virtual bool isSortable() { return true; }

	virtual SortOrder sortOrder() const;

	virtual void sort( const size_t& column, const SortOrder& );

	virtual bool isColumnSortable( const size_t& columnIndex ) const;

	ModelIndex mapToSource( const ModelIndex& ) const;

	ModelIndex mapToProxy( const ModelIndex& sourceIndex ) const;

	ModelRole sortRole() const;

	void setSortRrole( ModelRole role );

	bool lessThan( const ModelIndex& index1, const ModelIndex& index2 ) const;

	std::shared_ptr<Model> getSource() const;

	virtual bool classModelRoleEnabled();

  private:
	// NOTE: The data() of indexes points to the corresponding Mapping object for that index.
	struct Mapping {
		std::vector<int> sourceRows;
		std::vector<int> proxyRows;
		ModelIndex sourceParent;
	};

	using InternalMapIterator = UnorderedMap<ModelIndex, std::shared_ptr<Mapping>>::iterator;

	SortingProxyModel( std::shared_ptr<Model> );

	virtual void onModelUpdated( unsigned );

	Model& source();

	const Model& source() const;

	void sortMapping( Mapping&, int column, SortOrder );

	InternalMapIterator buildMapping( const ModelIndex& proxyIndex );

	void invalidate( unsigned flags = Model::UpdateFlag::DontInvalidateIndexes );

	void setSortingCaseSensitive( bool b );

	bool isSortingCaseSensitive();

	std::shared_ptr<Model> mSource;
	UnorderedMap<ModelIndex, std::shared_ptr<Mapping>> mMappings;
	int mKeyColumn{ -1 };
	SortOrder mSortOrder{ SortOrder::Ascending };
	ModelRole mSortRole{ ModelRole::Sort };
	bool mSortingCaseSensitive{ false };
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_SORTINGPROXYMODEL_HPP
