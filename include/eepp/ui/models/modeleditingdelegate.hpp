#ifndef EE_UI_MODELEDITINGDELEGATE_HPP
#define EE_UI_MODELEDITINGDELEGATE_HPP

#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <memory>

namespace EE { namespace UI { namespace Models {

class EE_API ModelEditingDelegate {
  public:
	enum SelectionBehavior {
		DoNotSelect,
		SelectAll,
	};

	virtual ~ModelEditingDelegate() = default;

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
	std::function<void()> onWillBeginEditing;
	std::function<void()> onValueSet;

	virtual Variant getValue() const = 0;

	virtual void setValue( const Variant& ) = 0;

	virtual void willBeginEditing() {
		if ( onWillBeginEditing )
			onWillBeginEditing();
	}

	ModelIndex const& index() const { return mIndex; }

	ModelEditingDelegate::SelectionBehavior getSelectionBehavior() const {
		return mSelectionBehavior;
	}

	void setSelectionBehavior( SelectionBehavior selectionBehavior ) {
		mSelectionBehavior = selectionBehavior;
	}

	inline ModelRole pullDataFrom() const { return mPullDataFrom; }

	inline void setPullDataFrom( ModelRole newPullDataFrom ) { mPullDataFrom = newPullDataFrom; }

  protected:
	ModelEditingDelegate() = default;

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
	ModelRole mPullDataFrom{ ModelRole::Display };

  protected:
	SelectionBehavior mSelectionBehavior{ SelectionBehavior::SelectAll };
};

class EE_API StringModelEditingDelegate : public ModelEditingDelegate {
  public:
	static StringModelEditingDelegate* New() { return eeNew( StringModelEditingDelegate, () ); }

	StringModelEditingDelegate() = default;

	virtual ~StringModelEditingDelegate() = default;

	Variant getValue() const override {
		return Variant( getWidget()->asType<UITextInput>()->getText() );
	};

	void setValue( const Variant& val ) override {
		getWidget()->asType<UITextInput>()->setText( val.toString() );
	}

	void willBeginEditing() override {
		if ( mSelectionBehavior == SelectionBehavior::SelectAll )
			getWidget()->asType<UITextInput>()->getDocument().selectAll();

		ModelEditingDelegate::willBeginEditing();
	}

  protected:
	UIWidget* createWidget() override {
		auto input = UITextInput::New();
		input->addClass( "table_cell_edit" );
		input->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		input->addEventListener( Event::OnPressEnter, [this]( auto ) { commit(); } );
		input->addEventListener( Event::OnFocusLoss, [this]( auto ) { rollback(); } );
		input->addEventListener( Event::KeyUp, [this]( const Event* event ) {
			if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
				rollback();
		} );
		input->addEventListener( Event::OnValueChange, [this]( auto ) { change(); } );
		return input;
	}
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELEDITINGDELEGATE_HPP
