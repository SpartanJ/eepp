#ifndef EE_UI_MODELEDITINGDELEGATE_HPP
#define EE_UI_MODELEDITINGDELEGATE_HPP

#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <memory>

namespace EE { namespace UI { namespace Models {

class EE_API ModelEditingDelegate {
  public:
	virtual ~ModelEditingDelegate() {}

	void bind( std::shared_ptr<Model> model, const ModelIndex& index ) {
		if ( mModel.get() == model.get() && mIndex == index )
			return;
		mModel = model;
		mIndex = index;
		mWidget = createWidget();
	}

	UIWidget* getWidget() { return mWidget; }
	UIWidget* getWidget() const { return mWidget; }

	std::function<void()> onCommit;

	virtual Variant getValue() const = 0;
	virtual void setValue( const Variant& ) = 0;
	virtual void willBeginEditing() {}

  protected:
	ModelEditingDelegate() {}

	virtual UIWidget* createWidget() = 0;

	void commit() {
		if ( onCommit )
			onCommit();
	}

	std::shared_ptr<Model> mModel;
	ModelIndex mIndex;
	UIWidget* mWidget;
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELEDITINGDELEGATE_HPP
