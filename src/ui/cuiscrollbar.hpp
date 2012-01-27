#ifndef EE_UICUISCROLLBAR_HPP
#define EE_UICUISCROLLBAR_HPP

#include "cuicomplexcontrol.hpp"
#include "cuislider.hpp"

namespace EE { namespace UI {

class cUIScrollBar : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					VerticalScrollBar( false )
				{
				}

				inline ~CreateParams() {}

				bool 	VerticalScrollBar;
		};

		cUIScrollBar( const cUIScrollBar::CreateParams& Params );

		virtual ~cUIScrollBar();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

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

		virtual void Update();

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

		virtual void OnAlphaChange();

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		void ManageClick( const Uint32& Flags );
};

}}

#endif

