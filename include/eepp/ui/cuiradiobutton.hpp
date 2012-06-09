#ifndef EE_UICUIRADIOBUTTON_H
#define EE_UICUIRADIOBUTTON_H

#include <eepp/ui/cuitextbox.hpp>
#include <eepp/ui/cuipushbutton.hpp>

namespace EE { namespace UI {

class EE_API cUIRadioButton : public cUITextBox {
	public:
		cUIRadioButton( const cUITextBox::CreateParams& Params );

		virtual ~cUIRadioButton();

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

		void AutoActivate();

		bool CheckActives();

		virtual void OnAlphaChange();

		virtual Uint32 OnKeyDown( const cUIEventKey& Event );

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		virtual void AutoSize();
};

}}

#endif


