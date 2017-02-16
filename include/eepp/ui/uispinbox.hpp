#ifndef EE_UICUISPINBOX_HPP
#define EE_UICUISPINBOX_HPP

#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UISpinBox : public UIComplexControl {
	public:
		class CreateParams : public UITextInput::CreateParams {
			public:
				inline CreateParams() :
					UITextInput::CreateParams(),
					DefaultValue( 0.f ),
					AllowDotsInNumbers( false )
				{
					MaxLength = 24;
				}

				inline ~CreateParams() {}

				Float DefaultValue;
				bool AllowDotsInNumbers;
		};

		UISpinBox( const UISpinBox::CreateParams& Params );

		virtual ~UISpinBox();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual void padding( const Recti& padding );

		const Recti& padding() const;

		virtual void clickStep( const Float& step );

		const Float& clickStep() const;

		virtual Uint32 onMessage( const UIMessage * Msg );

		void addValue( const Float& value );

		virtual void minValue( const Float& MinVal );

		const Float& minValue() const;

		virtual void maxValue( const Float& MaxVal );

		const Float& maxValue() const;

		virtual void value( const Float& Val );

		const Float& value() const;

		virtual void update();

		UIControlAnim * getButtonPushUp() const;

		UIControlAnim * getButtonPushDown() const;

		UITextInput * getTextInput() const;
	protected:
		UITextInput * 		mInput;
		UIControlAnim * 	mPushUp;
		UIControlAnim * 	mPushDown;
		Float				mMinValue;
		Float				mMaxValue;
		Float				mValue;
		Float				mClickStep;

		void adjustChilds();

		void internalValue( const Float& Val, const bool& Force = false );
		
		virtual void onAlphaChange();
};

}}

#endif
