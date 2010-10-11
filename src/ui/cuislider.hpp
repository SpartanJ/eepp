#ifndef EE_UIcUISlider_HPP
#define EE_UIcUISlider_HPP

#include "cuicontrolanim.hpp"
#include "cuisliderbutton.hpp"

namespace EE { namespace UI {

class EE_API cUISlider : public cUIControlAnim {
	public:
		class CreateParams : public cUIControl::CreateParams {
			public:
				inline CreateParams() : cUIControl::CreateParams() {
					VerticalSlider = false;
				}

				inline ~CreateParams() {}

				bool 	VerticalSlider;
		};

		cUISlider( const cUISlider::CreateParams& Params );

		~cUISlider();

		virtual void SetTheme( cUITheme * Theme );

		virtual void Value( const eeFloat& Val );

		const eeFloat& Value() const;

		virtual void MinValue( const eeFloat& MinVal );

		const eeFloat& MinValue() const;

		virtual void MaxValue( const eeFloat& MaxVal );

		const eeFloat& MaxValue() const;

		virtual void ClickStep( const eeFloat& step );

		const eeFloat& ClickStep() const;

		const bool& IsVertical() const;
		
		virtual void Update();
	protected:
		friend class Private::cUISliderButton;

		bool				mVertical;
		cUIControl *		mBackSlider;
		cUIDragable * 		mSlider;
		eeFloat				mMinValue;
		eeFloat				mMaxValue;
		eeFloat				mValue;
		eeFloat				mClickStep;

		bool				mOnPosChange;

		void FixSliderPos();
		
		void ManageClick( const Uint32& Flags );
};

}}

#endif


