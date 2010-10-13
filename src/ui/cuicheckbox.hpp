#ifndef EE_UICUICHECKBOX_H
#define EE_UICUICHECKBOX_H

#include "cuitextbox.hpp"
#include "cuipushbutton.hpp"

namespace EE { namespace UI {

class EE_API cUICheckBox : public cUITextBox {
	public:
		cUICheckBox( const cUITextBox::CreateParams& Params );

		~cUICheckBox();

		virtual void SetTheme( cUITheme * Theme );

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		const bool& IsActive() const;

		void Active( const bool& active );

		const bool& Active() const;

		virtual void Padding( const eeRectf& padding );
		
		cUIControlAnim * ActiveButton() const;
		
		cUIControlAnim * InactiveButton() const;
	protected:
		cUIControlAnim *	mActiveButton;
		cUIControlAnim *	mInactiveButton;
		bool				mActive;

		void SwitchState();
};

}}

#endif


