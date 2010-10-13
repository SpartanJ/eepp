#ifndef EE_UICUISPINBOX_HPP
#define EE_UICUISPINBOX_HPP

#include "cuitextinput.hpp"
#include "cuipushbutton.hpp"

namespace EE { namespace UI {

class EE_API cUISpinBox : public cUIControlAnim {
	public:
		class CreateParams : public cUITextInput::CreateParams {
			public:
				inline CreateParams() :
					cUITextInput::CreateParams()
				{
					MaxLenght = 24;
					DefaultValue = 0.f;
				}

				inline ~CreateParams() {}
				
				eeFloat DefaultValue;
				bool AllowDotsInNumbers;
		};

		cUISpinBox( const cUISpinBox::CreateParams& Params );

		~cUISpinBox();

		virtual void SetTheme( cUITheme * Theme );
		
		virtual void Padding( const eeRectf& padding );

		const eeRectf& Padding() const;
		
		virtual void ClickStep( const eeFloat& step );

		const eeFloat& ClickStep() const;
		
		virtual Uint32 OnMessage( const cUIMessage * Msg );
		
		void AddValue( const eeFloat& value );
		
		virtual void MinValue( const eeFloat& MinVal );

		const eeFloat& MinValue() const;

		virtual void MaxValue( const eeFloat& MaxVal );

		const eeFloat& MaxValue() const;
		
		virtual void Value( const eeFloat& Val );

		const eeFloat& Value() const;
		
		virtual void Update();
		
		cUIControlAnim * ButtonPushUp() const;
		
		cUIControlAnim * ButtonPushDown() const;
		
		cUITextInput * TextInput() const;
	protected:
		cUITextInput * 		mInput;
		cUIControlAnim * 	mPushUp;
		cUIControlAnim * 	mPushDown;
		eeFloat				mMinValue;
		eeFloat				mMaxValue;
		eeFloat				mValue;
		eeFloat				mClickStep;
		
		void AdjustChilds();
		
		void InternalValue( const eeFloat& Val, const bool& Force = false );

};

}}

#endif
