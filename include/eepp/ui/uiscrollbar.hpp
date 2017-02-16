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

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void value( Float Val );

		const Float& value() const;

		virtual void minValue( const Float& MinVal );

		const Float& minValue() const;

		virtual void maxValue( const Float& MaxVal );

		const Float& maxValue() const;

		virtual void clickStep( const Float& step );

		const Float& clickStep() const;

		virtual void setTheme( UITheme * Theme );

		const bool& isVertical() const;

		virtual void update();

		UISlider * getSlider() const;

		UIControlAnim * getButtonUp() const;

		UIControlAnim * getButtonDown() const;
	protected:
		UISlider * 		mSlider;
		UIControlAnim *	mBtnUp;
		UIControlAnim * 	mBtnDown;

		virtual void onSizeChange();

		void adjustChilds();

		void onValueChangeCb( const UIEvent * Event );

		virtual void onAlphaChange();

		virtual Uint32 onMessage( const UIMessage * Msg );

		void manageClick( const Uint32& flags );
};

}}

#endif

