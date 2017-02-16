#ifndef EE_UICUIRADIOBUTTON_H
#define EE_UICUIRADIOBUTTON_H

#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UIRadioButton : public UITextBox {
	public:
		UIRadioButton( const UITextBox::CreateParams& Params );

		virtual ~UIRadioButton();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		const bool& isActive() const;

		void active( const bool& active );

		const bool& active() const;

		virtual void padding( const Recti& padding );

		UIControlAnim * activeButton() const;

		UIControlAnim * inactiveButton() const;
	protected:
		UIControlAnim *	mActiveButton;
		UIControlAnim *	mInactiveButton;
		bool				mActive;
		Uint32				mLastTick;

		virtual void onSizeChange();

		void switchState();

		void autoActivate();

		bool checkActives();

		virtual void onAlphaChange();

		virtual Uint32 onKeyDown( const UIEventKey& Event );

		virtual Uint32 onMessage( const UIMessage * Msg );

		virtual void autoSize();
};

}}

#endif


