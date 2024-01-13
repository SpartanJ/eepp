#ifndef EE_UI_MODELS_WIDGETTREEMODEL_HPP
#define EE_UI_MODELS_WIDGETTREEMODEL_HPP

#include <eepp/scene/node.hpp>
#include <eepp/ui/models/model.hpp>
#include <memory>

using namespace EE::UI;

namespace EE { namespace UI { namespace Models {

class EE_API WidgetTreeModel : public Model {
  public:
	enum Column {
		Type = 0,
		ID,
		Classes,
		PseudoClasses,
		Count,
	};

	static std::shared_ptr<WidgetTreeModel> New( Node* node );

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const override;

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const override;

	virtual std::string columnName( const size_t& /*column*/ ) const override;

	virtual Variant data( const ModelIndex&, ModelRole role = ModelRole::Display ) const override;

	virtual ModelIndex index( int row, int column,
							  const ModelIndex& parent = ModelIndex() ) const override;

	virtual ModelIndex parentIndex( const ModelIndex& ) const override;

	ModelIndex getRoot() const;

	ModelIndex getModelIndex( const Node* node ) const;

  protected:
	Node* mRoot;

	WidgetTreeModel( Node* node );
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_WIDGETTREEMODEL_HPP
