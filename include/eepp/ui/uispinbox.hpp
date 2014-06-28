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

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		virtual void Padding( const Recti& padding );

		const Recti& Padding() const;

		virtual void ClickStep( const Float& step );

		const Float& ClickStep() const;

		virtual Uint32 OnMessage( const UIMessage * Msg );

		void AddValue( const Float& value );

		virtual void MinValue( const Float& MinVal );

		const Float& MinValue() const;

		virtual void MaxValue( const Float& MaxVal );

		const Float& MaxValue() const;

		virtual void Value( const Float& Val );

		const Float& Value() const;

		virtual void Update();

		UIControlAnim * ButtonPushUp() const;

		UIControlAnim * ButtonPushDown() const;

		UITextInput * TextInput() const;
	protected:
		UITextInput * 		mInput;
		UIControlAnim * 	mPushUp;
		UIControlAnim * 	mPushDown;
		Float				mMinValue;
		Float				mMaxValue;
		Float				mValue;
		Float				mClickStep;

		void AdjustChilds();

		void InternalValue( const Float& Val, const bool& Force = false );
		
		virtual void OnAlphaChange();
};

}}

#endif
