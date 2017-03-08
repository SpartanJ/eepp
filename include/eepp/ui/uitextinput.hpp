#ifndef EE_UICUITEXTINPUT_H
#define EE_UICUITEXTINPUT_H

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/window/inputtextbuffer.hpp>

namespace EE { namespace UI {

class EE_API UITextInput : public UITextView {
	public:
		static UITextInput * New();

		UITextInput();

		virtual ~UITextInput();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void update();

		virtual void draw();

		void pushIgnoredChar( const Uint32& ch );

		virtual void setTheme( UITheme * Theme );

		InputTextBuffer * getInputTextBuffer();

		UITextInput * setAllowEditing( const bool& allow );

		const bool& getAllowEditing() const;

		virtual const String& getText();

		virtual UITextView * setText( const String& text );

		virtual void shrinkText( const Uint32& MaxWidth );

		UITextInput * setMaxLength( Uint32 maxLength );

		Uint32 getMaxLength();

		UITextInput * setFreeEditing( bool support );

		bool isFreeEditingEnabled();
	protected:
		InputTextBuffer	mTextBuffer;
		Float			mWaitCursorTime;
		Vector2f		mCurPos;
		int				mCursorPos;
		bool			mAllowEditing;
		bool			mShowingWait;

		void resetWaitCursor();

		virtual void alignFix();

		virtual void onAutoSize();

		void privOnPressEnter();

		void autoPadding();

		virtual Uint32 onMouseClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseExit( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onFocus();

		virtual Uint32 onFocusLoss();

		virtual Uint32 onPressEnter();

		void onThemeLoaded();

		virtual void onCursorPosChange();

		void drawWaitingCursor();

		virtual void updateText();

		virtual void selCurInit( const Int32& init );

		virtual void selCurEnd( const Int32& end );

		virtual Int32 selCurInit();

		virtual Int32 selCurEnd();
};

}}

#endif
