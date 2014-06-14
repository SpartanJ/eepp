#ifndef EE_UIcUISlider_HPP
#define EE_UIcUISlider_HPP

#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/ui/cuisliderbutton.hpp>

namespace EE { namespace UI {

class EE_API cUISlider : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
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

		cUISlider( const cUISlider::CreateParams& Params );

		virtual ~cUISlider();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

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

		cUIControl * GetBackSlider() const;

		cUIDragable * GetSliderButton() const;

		void AdjustChilds();

		const bool& AllowHalfSliderOut() const;

		const bool& ExpandBackground() const;

		void ManageClick( const Uint32& Flags );
	protected:
		friend class Private::cUISliderButton;

		bool				mVertical;
		bool				mAllowHalfSliderOut;
		bool				mExpandBackground;
		cUIControlAnim *	mBackSlider;
		cUIControlAnim * 	mSlider;
		Float				mMinValue;
		Float				mMaxValue;
		Float				mValue;
		Float				mClickStep;

		bool				mOnPosChange;

		Uint32				mLastTickMove;

		virtual void OnSizeChange();

		void FixSliderPos();

		virtual Uint32 OnKeyDown( const cUIEventKey &Event );
		
		virtual void OnAlphaChange();
};

}}

#endif


