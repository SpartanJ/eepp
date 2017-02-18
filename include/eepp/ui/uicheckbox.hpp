#ifndef EE_UICUICHECKBOX_H
#define EE_UICUICHECKBOX_H

#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UICheckBox : public UITextBox {
	public:
		UICheckBox( const UITextBox::CreateParams& Params );

		virtual ~UICheckBox();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		const bool& isActive() const;

		void setActive( const bool& active );

		virtual void setPadding( const Recti& padding );

		UIControlAnim * getActiveButton() const;

		UIControlAnim * getInactiveButton() const;
	protected:
		UIControlAnim *	mActiveButton;
		UIControlAnim *	mInactiveButton;
		bool			mActive;
		Uint32			mLastTick;

		virtual void onSizeChange();

		void switchState();

		virtual void onAlphaChange();

		virtual Uint32 onKeyDown( const UIEventKey& Event );

		virtual Uint32 onMessage( const UIMessage * Msg );

		void doAftersetTheme();

		virtual void autoSize();
};

}}

#endif


