#ifndef EE_UICUICHECKBOX_H
#define EE_UICUICHECKBOX_H

#include "cuitextbox.hpp"
#include "cuipushbutton.hpp"

namespace EE { namespace UI {

class EE_API cUICheckBox : public cUITextBox {
	public:
		cUICheckBox( const cUITextBox::CreateParams& Params );

		virtual ~cUICheckBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		const bool& IsActive() const;

		void Active( const bool& active );

		const bool& Active() const;

		virtual void Padding( const eeRecti& padding );

		cUIControlAnim * ActiveButton() const;

		cUIControlAnim * InactiveButton() const;
	protected:
		cUIControlAnim *	mActiveButton;
		cUIControlAnim *	mInactiveButton;
		bool				mActive;
		Uint32				mLastTick;

		virtual void OnSizeChange();

		void SwitchState();

		virtual void OnAlphaChange();

		virtual Uint32 OnKeyDown( const cUIEventKey& Event );

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		void DoAfterSetTheme();

		virtual void AutoSize();
};

}}

#endif


