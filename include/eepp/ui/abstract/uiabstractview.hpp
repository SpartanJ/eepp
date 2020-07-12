#ifndef EE_UI_UIABSTRACTVIEW_HPP
#define EE_UI_UIABSTRACTVIEW_HPP

#include <eepp/ui/abstract/model.hpp>
#include <eepp/ui/abstract/modeleditingdelegate.hpp>
#include <eepp/ui/abstract/modelselection.hpp>
#include <eepp/ui/uiscrollablewidget.hpp>
#include <memory>

namespace EE { namespace UI { namespace Abstract {

class ModelEvent : public Event {
  public:
	ModelEvent( Model* model, const ModelIndex& index, Node* node ) :
		Event( node, Event::OnModelEvent ), model( model ), index( index ) {}

	const Model* getModel() const { return model; }

	const ModelIndex& getModelIndex() const { return index; }

  protected:
	const Model* model;
	ModelIndex index;
};

class EE_API UIAbstractView : public UIScrollableWidget {
  public:
	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	void setModel( std::shared_ptr<Model> );

	Model* getModel() { return mModel.get(); }

	const Model* getModel() const { return mModel.get(); }

	ModelSelection& getSelection() { return mSelection; }

	const ModelSelection& getSelection() const { return mSelection; }

	virtual void selectAll() = 0;

	void notifySelectionChange();

	std::function<void()> getOnSelectionChange() const;

	void setOnSelectionChange( const std::function<void()>& onSelectionChange );

	std::function<void( const ModelIndex& )> getOnSelection() const;

	void setOnSelection( const std::function<void( const ModelIndex& )>& onSelection );

  protected:
	friend class Model;

	virtual void onModelUpdate( unsigned flags );

	virtual void onModelSelectionChange();

	UIAbstractView( const std::string& tag );

	virtual ~UIAbstractView();

	bool mEditable{false};
	ModelIndex mEditIndex;
	UIWidget* mEditWidget;
	Rect mEditWidgetContentRect;

	std::shared_ptr<Model> mModel;
	std::unique_ptr<ModelEditingDelegate> mEditingDelegate;
	ModelSelection mSelection;

	std::function<void()> mOnSelectionChange;
	std::function<void( const ModelIndex& )> mOnSelection;
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTVIEW_HPP
