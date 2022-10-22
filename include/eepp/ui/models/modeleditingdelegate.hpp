#ifndef EE_UI_MODELEDITINGDELEGATE_HPP
#define EE_UI_MODELEDITINGDELEGATE_HPP

#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <memory>

namespace EE { namespace UI { namespace Models {

class EE_API ModelEditingDelegate {
  public:
	enum SelectionBehavior {
		DoNotSelect,
		SelectAll,
	};

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
	std::function<void()> onRollback;
	std::function<void()> onChange;

	virtual Variant getValue() const = 0;
	virtual void setValue( const Variant&,
						   SelectionBehavior selectionBehavior = SelectionBehavior::SelectAll ) = 0;
	virtual void willBeginEditing() {}

  protected:
	ModelEditingDelegate() {}

	virtual UIWidget* createWidget() = 0;

	void commit() {
		if ( onCommit )
			onCommit();
	}

	void rollback() {
		if ( onRollback )
			onRollback();
	}
	void change() {
		if ( onChange )
			onChange();
	}

  private:
	std::shared_ptr<Model> mModel;
	ModelIndex mIndex;
	UIWidget* mWidget{ nullptr };
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELEDITINGDELEGATE_HPP
