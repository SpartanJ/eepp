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

	virtual Variant data( const ModelIndex&, Role = Role::Display ) const;

	virtual void update();

	virtual int keyColumn() const;

	virtual SortOrder sortOrder() const;

	virtual void setKeyColumnAndSortOrder( const size_t&, const SortOrder& );

	virtual bool isColumnSortable( const size_t& columnIndex ) const;

	ModelIndex mapToTarget( const ModelIndex& ) const;

	Role sortRole() const;

	void setSortRrole( Role role );

  private:
	SortingProxyModel( std::shared_ptr<Model> );

	virtual void onModelUpdated( unsigned );

	Model& target();

	const Model& target() const;

	void resort( unsigned flags = Model::UpdateFlag::DontInvalidateIndexes );

	void setSortingCaseSensitive( bool b );

	bool isSortingCaseSensitive();

	std::shared_ptr<Model> mTarget;
	std::vector<int> mRowMappings;
	int mKeyColumn{-1};
	SortOrder mSortOrder{SortOrder::Ascending};
	Role mSortRole{Role::Sort};
	bool mSortingCaseSensitive{false};
	bool mSorting{false};
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_SORTINGPROXYMODEL_HPP
