#ifndef EE_UICUISCROLLBAR_HPP
#define EE_UICUISCROLLBAR_HPP

#include "cuicontrolanim.hpp"
#include "cuislider.hpp"

namespace EE { namespace UI {

class cUIScrollBar : public cUIControlAnim {
	public:
		class CreateParams : public cUIControl::CreateParams {
			public:
				inline CreateParams() : cUIControl::CreateParams() {
					VerticalScrollBar = false;
				}

				inline ~CreateParams() {}

				bool 	VerticalScrollBar;
		};

		cUIScrollBar( const cUIScrollBar::CreateParams& Params );

		~cUIScrollBar();

		virtual void Value( eeFloat Val );

		const eeFloat& Value() const;

		virtual void MinValue( const eeFloat& MinVal );

		const eeFloat& MinValue() const;

		virtual void MaxValue( const eeFloat& MaxVal );

		const eeFloat& MaxValue() const;

		virtual void ClickStep( const eeFloat& step );

		const eeFloat& ClickStep() const;

		virtual void SetTheme( cUITheme * Theme );

		const bool& IsVertical() const;

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		virtual void Update();

		void ManageClick( const Uint32& Flags );

		cUISlider * Slider() const;

		cUIControlAnim * ButtonUp() const;

		cUIControlAnim * ButtonDown() const;
	protected:
		cUISlider * 		mSlider;
		cUIControlAnim *	mBtnUp;
		cUIControlAnim * 	mBtnDown;

		virtual void OnSizeChange();

		void AdjustChilds();

		void OnValueChangeCb( const cUIEvent * Event );
};

}}

#endif

