#ifndef EE_UICUISCROLLBAR_HPP
#define EE_UICUISCROLLBAR_HPP

#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uislider.hpp>

namespace EE { namespace UI {

class EE_API UIScrollBar : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					VerticalScrollBar( false )
				{
				}

				inline ~CreateParams() {}

				bool 	VerticalScrollBar;
		};

		UIScrollBar( const UIScrollBar::CreateParams& Params );

		virtual ~UIScrollBar();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Value( Float Val );

		const Float& Value() const;

		virtual void MinValue( const Float& MinVal );

		const Float& MinValue() const;

		virtual void MaxValue( const Float& MaxVal );

		const Float& MaxValue() const;

		virtual void ClickStep( const Float& step );

		const Float& ClickStep() const;

		virtual void SetTheme( UITheme * Theme );

		const bool& IsVertical() const;

		virtual void Update();

		UISlider * Slider() const;

		UIControlAnim * ButtonUp() const;

		UIControlAnim * ButtonDown() const;
	protected:
		UISlider * 		mSlider;
		UIControlAnim *	mBtnUp;
		UIControlAnim * 	mBtnDown;

		virtual void OnSizeChange();

		void AdjustChilds();

		void OnValueChangeCb( const UIEvent * Event );

		virtual void OnAlphaChange();

		virtual Uint32 OnMessage( const UIMessage * Msg );

		void ManageClick( const Uint32& Flags );
};

}}

#endif

