#ifndef EE_UI_UIABSTRACTVIEW_HPP
#define EE_UI_UIABSTRACTVIEW_HPP

#include <eepp/ui/abstract/model.hpp>
#include <eepp/ui/abstract/modeleditingdelegate.hpp>
#include <eepp/ui/abstract/modelselection.hpp>
#include <eepp/ui/uiscrollablewidget.hpp>
#include <memory>

namespace EE { namespace UI { namespace Abstract {

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

	bool isEditable() const { return mEditable; }

	void setEditable( bool editable ) { mEditable = editable; }

	void setActivatesOnSelection( bool b ) { mActivatesOnSelection = b; }

	bool getActivatesOnSelection() const { return mActivatesOnSelection; }

	void notifySelectionChange();

  protected:
	friend class Model;

	virtual void onModelUpdate( unsigned flags );

	virtual void onModelSelectionChange();

	UIAbstractView( const std::string& tag );

	virtual ~UIAbstractView();

	void setHoveredIndex( const ModelIndex& );
	void activate( const ModelIndex& );
	void activateSelected();

	bool mEditable{false};
	ModelIndex mEditIndex;
	UIWidget* mEditWidget;
	Rect mEditWidgetContentRect;

	Vector2i mLeftMouseDownPosition;
	bool mMightDrag{false};

	ModelIndex mHoveredIndex;

	std::shared_ptr<Model> mModel;
	std::unique_ptr<ModelEditingDelegate> mEditingDelegate;
	ModelSelection mSelection;
	bool mActivatesOnSelection{false};

	std::function<void()> mOnSelectionChange;
	std::function<void( const ModelIndex& )> mOnActivation;
	std::function<void( const ModelIndex& )> mOnSelection;
	std::function<void( const ModelIndex&, const DropEvent& )> mOnDrop;
};

}}} // namespace EE::UI::Abstract

#endif // EE_UI_UIABSTRACTVIEW_HPP
