#ifndef EE_UICUISPINBOX_HPP
#define EE_UICUISPINBOX_HPP

#include <eepp/ui/cuitextinput.hpp>
#include <eepp/ui/cuipushbutton.hpp>

namespace EE { namespace UI {

class EE_API cUISpinBox : public cUIComplexControl {
	public:
		class CreateParams : public cUITextInput::CreateParams {
			public:
				inline CreateParams() :
					cUITextInput::CreateParams(),
					DefaultValue( 0.f ),
					AllowDotsInNumbers( false )
				{
					MaxLength = 24;
				}

				inline ~CreateParams() {}

				Float DefaultValue;
				bool AllowDotsInNumbers;
		};

		cUISpinBox( const cUISpinBox::CreateParams& Params );

		virtual ~cUISpinBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		virtual void Padding( const Recti& padding );

		const Recti& Padding() const;

		virtual void ClickStep( const Float& step );

		const Float& ClickStep() const;

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		void AddValue( const Float& value );

		virtual void MinValue( const Float& MinVal );

		const Float& MinValue() const;

		virtual void MaxValue( const Float& MaxVal );

		const Float& MaxValue() const;

		virtual void Value( const Float& Val );

		const Float& Value() const;

		virtual void Update();

		cUIControlAnim * ButtonPushUp() const;

		cUIControlAnim * ButtonPushDown() const;

		cUITextInput * TextInput() const;
	protected:
		cUITextInput * 		mInput;
		cUIControlAnim * 	mPushUp;
		cUIControlAnim * 	mPushDown;
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
