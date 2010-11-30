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
					AllowHalfSliderOut = true;
					ExpandBackground = false;
				}

				inline ~CreateParams() {}

				bool 	VerticalSlider;
				bool	AllowHalfSliderOut;
				bool	ExpandBackground;
		};

		cUISlider( const cUISlider::CreateParams& Params );

		~cUISlider();

		virtual void SetTheme( cUITheme * Theme );

		virtual void Value( eeFloat Val );

		const eeFloat& Value() const;

		virtual void MinValue( const eeFloat& MinVal );

		const eeFloat& MinValue() const;

		virtual void MaxValue( const eeFloat& MaxVal );

		const eeFloat& MaxValue() const;

		virtual void ClickStep( const eeFloat& step );

		const eeFloat& ClickStep() const;

		const bool& IsVertical() const;

		virtual void Update();

		cUIControl * GetBackSlider() const;

		cUIDragable * GetSliderButton() const;

		void AdjustChilds();

		const bool& AllowHalfSliderOut() const;

		const bool& ExpandBackground() const;
	protected:
		friend class Private::cUISliderButton;
		friend class cUIListBoxItem;

		bool				mVertical;
		bool				mAllowHalfSliderOut;
		bool				mExpandBackground;
		cUIControl *		mBackSlider;
		cUIDragable * 		mSlider;
		eeFloat				mMinValue;
		eeFloat				mMaxValue;
		eeFloat				mValue;
		eeFloat				mClickStep;

		bool				mOnPosChange;

		Uint32				mLastTickMove;

		virtual void OnSizeChange();

		void FixSliderPos();

		void ManageClick( const Uint32& Flags );

		void ManageKeyboard();
};

}}

#endif


