#ifndef EE_UICUIMESSAGEBOX_HPP
#define EE_UICUIMESSAGEBOX_HPP

#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI {

class UITextEdit;
class UITextInput;
class UILayout;
class UIPushButton;

#define UI_MESSAGE_BOX_DEFAULT_FLAGS                                          \
	UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS | UI_WIN_MODAL | \
		UI_WIN_SHARE_ALPHA_WITH_CHILDS

class EE_API UIMessageBox : public UIWindow {
  public:
	enum Type { OK_CANCEL, YES_NO, RETRY_CANCEL, OK, INPUT, TEXT_EDIT };

	static UIMessageBox* New( const Type& type, const String& message,
							  const Uint32& windowFlags = UI_MESSAGE_BOX_DEFAULT_FLAGS );

	UIMessageBox( const Type& type, const String& message,
				  const Uint32& windowFlags = UI_MESSAGE_BOX_DEFAULT_FLAGS );

	virtual ~UIMessageBox();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void setTheme( UITheme* theme );

	UITextView* getTextBox() const;

	UIPushButton* getButtonOK() const;

	UIPushButton* getButtonCancel() const;

	virtual bool show();

	const KeyBindings::Shortcut& getCloseShortcut() const;

	void setCloseShortcut( const KeyBindings::Shortcut& closeWithKey );

	UITextInput* getTextInput() const;

	UITextEdit* getTextEdit() const;

	UILayout* getLayoutCont() const;

  protected:
	Type mMsgBoxType;
	UITextView* mTextBox{ nullptr };
	UIPushButton* mButtonOK{ nullptr };
	UIPushButton* mButtonCancel{ nullptr };
	UITextInput* mTextInput{ nullptr };
	UITextEdit* mTextEdit{ nullptr };
	KeyBindings::Shortcut mCloseShortcut;
	UILayout* mLayoutCont{ nullptr };

	virtual void onWindowReady();

	virtual Uint32 onKeyUp( const KeyEvent& event );
};

}} // namespace EE::UI

#endif
