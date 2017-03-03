#ifndef EE_UIUISlider_HPP
#define EE_UIUISlider_HPP

#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uisliderbutton.hpp>

namespace EE { namespace UI {

class EE_API UISlider : public UIComplexControl {
	public:
		static UISlider * New();

		UISlider();

		virtual ~UISlider();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual void setValue( Float Val );

		const Float& getValue() const;

		virtual void setMinValue( const Float& MinVal );

		const Float& getMinValue() const;

		virtual void setMaxValue( const Float& MaxVal );

		const Float& getMaxValue() const;

		virtual void setClickStep( const Float& step );

		const Float& getClickStep() const;

		bool isVertical() const;

		virtual void update();

		UIControl * getBackSlider() const;

		UIDragable * getSliderButton() const;

		void adjustChilds();

		void manageClick( const Uint32& flags );

		UI_ORIENTATION getOrientation() const;

		UISlider * setOrientation( const UI_ORIENTATION & orientation );

		bool getAllowHalfSliderOut() const;

		void setAllowHalfSliderOut( bool allowHalfSliderOut );

		bool getExpandBackground() const;

		void setExpandBackground( bool expandBackground );

		Float getPageStep() const;

		void setPageStep( const Float & pageStep );
	protected:
		friend class Private::UISliderButton;

		UI_ORIENTATION		mOrientation;
		SliderStyleConfig	mStyleConfig;
		UIControlAnim *		mBackSlider;
		Private::UISliderButton * 	mSlider;
		Float				mMinValue;
		Float				mMaxValue;
		Float				mValue;
		Float				mClickStep;
		Float				mPageStep;

		bool				mOnPosChange;

		Uint32				mLastTickMove;

		virtual void onSizeChange();

		void fixSliderPos();

		virtual Uint32 onKeyDown( const UIEventKey &Event );
		
		virtual void onAlphaChange();
};

}}

#endif


