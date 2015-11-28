#ifndef EE_UIUISlider_HPP
#define EE_UIUISlider_HPP

#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uisliderbutton.hpp>

namespace EE { namespace UI {

class EE_API UISlider : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					VerticalSlider( false ),
					AllowHalfSliderOut( true ),
					ExpandBackground( false )
				{
				}

				inline ~CreateParams() {}

				bool 	VerticalSlider;
				bool	AllowHalfSliderOut;
				bool	ExpandBackground;
		};

		UISlider( const UISlider::CreateParams& Params );

		virtual ~UISlider();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		virtual void Value( Float Val );

		const Float& Value() const;

		virtual void MinValue( const Float& MinVal );

		const Float& MinValue() const;

		virtual void MaxValue( const Float& MaxVal );

		const Float& MaxValue() const;

		virtual void ClickStep( const Float& step );

		const Float& ClickStep() const;

		const bool& IsVertical() const;

		virtual void Update();

		UIControl * GetBackSlider() const;

		UIDragable * GetSliderButton() const;

		void AdjustChilds();

		const bool& AllowHalfSliderOut() const;

		const bool& ExpandBackground() const;

		void ManageClick( const Uint32& Flags );
	protected:
		friend class Private::UISliderButton;

		bool				mVertical;
		bool				mAllowHalfSliderOut;
		bool				mExpandBackground;
		UIControlAnim *	mBackSlider;
		UIControlAnim * 	mSlider;
		Float				mMinValue;
		Float				mMaxValue;
		Float				mValue;
		Float				mClickStep;

		bool				mOnPosChange;

		Uint32				mLastTickMove;

		virtual void OnSizeChange();

		void FixSliderPos();

		virtual Uint32 OnKeyDown( const UIEventKey &Event );
		
		virtual void OnAlphaChange();
};

}}

#endif


