#ifndef EE_UICUIRADIOBUTTON_H
#define EE_UICUIRADIOBUTTON_H

#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UIRadioButton : public UITextView {
	public:
		static UIRadioButton * New();

		UIRadioButton();

		virtual ~UIRadioButton();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		const bool& isActive() const;

		void setActive( const bool& active );

		UINode * getActiveButton() const;

		UINode * getInactiveButton() const;

		Int32 getTextSeparation() const;

		void setTextSeparation(const Int32 & textSeparation);

		virtual void setAttribute( const NodeAttribute& attribute );
	protected:
		UINode *	mActiveButton;
		UINode *	mInactiveButton;
		bool			mActive;
		Uint32			mLastTick;
		Int32			mTextSeparation;

		virtual void onSizeChange();

		void switchState();

		void autoActivate();

		bool checkActives();

		virtual void onAlphaChange();

		virtual Uint32 onKeyDown( const KeyEvent& Event );

		virtual Uint32 onMessage( const NodeMessage * Msg );

		virtual void onAutoSize();

		virtual void onThemeLoaded();

		virtual void onPaddingChange();

		virtual void alignFix();
};

}}

#endif


