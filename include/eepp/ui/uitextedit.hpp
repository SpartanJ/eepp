#ifndef EE_UICUITEXTEDIT_HPP
#define EE_UICUITEXTEDIT_HPP

#include <eepp/ui/uicontrolanim.hpp>
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

		virtual void update();

		void setAllowEditing( const bool& allow );

		const bool& isEditingAllowed() const;

		void setVerticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getVerticalScrollMode();

		void setHorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& getHorizontalScrollMode();

		FontStyleConfig getFontStyleConfig() const;

		void setFontStyleConfig(const FontStyleConfig & fontStyleConfig);

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		UITextInput *		mTextInput;
		UIScrollBar *		mHScrollBar;
		UIScrollBar *		mVScrollBar;
		UI_SCROLLBAR_MODE	mHScrollBarMode;
		UI_SCROLLBAR_MODE	mVScrollBarMode;
		bool				mSkipValueChange;
		Recti				mContainerPadding;

		virtual void onSizeChange();

		virtual void onAlphaChange();

		virtual void onParentSizeChange( const Vector2i& SizeChange );

		void onVScrollValueChange( const UIEvent * Event );

		void onHScrollValueChange( const UIEvent * Event );

		void onInputSizeChange( const UIEvent * Event = NULL );

		void onCursorPosChange( const UIEvent * Event );

		void autoPadding();

		void scrollbarsSet();

		void fixScroll();

		void fixScrollToCursor();

		void shrinkText( const Uint32& Width );
};

}}

#endif
