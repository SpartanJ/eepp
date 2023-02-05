#ifndef EE_UI_UIABSTRACTVIEW_HPP
#define EE_UI_UIABSTRACTVIEW_HPP

#include <eepp/ui/models/model.hpp>
#include <eepp/ui/models/modeleditingdelegate.hpp>
#include <eepp/ui/models/modelselection.hpp>
#include <eepp/ui/uiscrollablewidget.hpp>
#include <memory>

using namespace EE::UI::Models;

namespace EE { namespace UI { namespace Abstract {

enum class EE_API ModelEventType { Open, OpenTree, CloseTree, OpenMenu };

class EE_API ModelEvent : public Event {
  public:
	ModelEvent( Model* model, const ModelIndex& index, Node* node,
				const ModelEventType& modelEventType = ModelEventType::Open,
				const Event* triggerEvent = nullptr ) :
		Event( node, EventType::OnModelEvent ),
		model( model ),
		index( index ),
		modelEventType( modelEventType ),
		triggerEvent( triggerEvent ) {}

	const Model* getModel() const { return model; }

	const ModelIndex& getModelIndex() const { return index; }

	const ModelEventType& getModelEventType() const { return modelEventType; }

	const Event* getTriggerEvent() const { return triggerEvent; }

  protected:
	const Model* model;
	ModelIndex index;
	ModelEventType modelEventType;
	const Event* triggerEvent{ nullptr };
};

class EE_API UIAbstractView : public UIScrollableWidget {
  public:
	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

	void setModel( const std::shared_ptr<Model>& );

	Model* getModel() { return mModel.get(); }

	const Model* getModel() const { return mModel.get(); }

	std::shared_ptr<Model> getModelShared() const { return mModel; }

	ModelSelection& getSelection() { return mSelection; }

	const ModelSelection& getSelection() const { return mSelection; }

	virtual void selectAll() = 0;

	void notifySelectionChange();

	std::function<void()> getOnSelectionChange() const;

	void setOnSelectionChange( const std::function<void()>& onSelectionChange );

	std::function<void( const ModelIndex& )> getOnSelection() const;

	void setOnSelection( const std::function<void( const ModelIndex& )>& onSelection );

	virtual ModelIndex findRowWithText( const std::string& text, const bool& caseSensitive = false,
										const bool& exactMatch = false ) const;

  protected:
	friend class EE::UI::Models::Model;

	virtual void onModelUpdate( unsigned flags );

	virtual void onModelSelectionChange();

	void modelUpdate( unsigned flags );

	UIAbstractView( const std::string& tag );

	virtual ~UIAbstractView();

	bool mEditable{ false };
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
