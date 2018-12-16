#ifndef EE_UICUITEXTEDIT_HPP
#define EE_UICUITEXTEDIT_HPP

#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiscrollbar.hpp>

namespace EE { namespace UI {

class EE_API UITextEdit : public UIWidget {
	public:
		static UITextEdit * New();

		UITextEdit();

		virtual ~UITextEdit();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		const String& getText() const;

		void setText( const String& Txt );

		UITextInput * getTextInput() const;

		UIScrollBar * getHScrollBar() const;

		UIScrollBar * getVScrollBar() const;

		virtual void update( const Time& time );

		void setAllowEditing( const bool& allow );

		const bool& isEditingAllowed() const;

		void setVerticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getVerticalScrollMode();

		void setHorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getHorizontalScrollMode();

		UIFontStyleConfig getFontStyleConfig() const;

		void setFontStyleConfig(const UIFontStyleConfig & fontStyleConfig);

		virtual bool setAttribute( const NodeAttribute& attribute );
	protected:
		UITextInput *		mTextInput;
		UIScrollBar *		mHScrollBar;
		UIScrollBar *		mVScrollBar;
		UI_SCROLLBAR_MODE	mHScrollBarMode;
		UI_SCROLLBAR_MODE	mVScrollBarMode;
		bool				mSkipValueChange;
		Rectf				mContainerPadding;

		virtual void onSizeChange();

		virtual void onAlphaChange();

		virtual void onParentSizeChange( const Vector2f& SizeChange );

		virtual void onPaddingChange();

		void onVScrollValueChange( const Event * Event );

		void onHScrollValueChange( const Event * Event );

		void onInputSizeChange( const Event * Event = NULL );

		void onCursorPosChange( const Event * Event );

		void autoPadding();

		void scrollbarsSet();

		void fixScroll();

		void fixScrollToCursor();

		void shrinkText( const Uint32& Width );
};

}}

#endif
