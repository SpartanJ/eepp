#ifndef EE_UIUISlider_HPP
#define EE_UIUISlider_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UISlider : public UIWidget {
	public:
		static UISlider * New();

		static UISlider * NewVertical();

		static UISlider * NewHorizontal();

		UISlider( const UI_ORIENTATION& orientation = UI_HORIZONTAL );

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

		UINode * getBackSlider() const;

		UINode * getSliderButton() const;

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

		virtual bool setAttribute( const StyleSheetProperty& attribute, const Uint32& state = UIState::StateFlagNormal );

		Sizef getMinimumSize();
	protected:
		UI_ORIENTATION		mOrientation;
		bool				mAllowHalfSliderOut;
		bool				mExpandBackground;
		UINode *			mBackSlider;
		UIWidget *			mSlider;
		Float				mMinValue;
		Float				mMaxValue;
		Float				mValue;
		Float				mClickStep;
		Float				mPageStep;

		bool				mOnPosChange;

		Uint32				mLastTickMove;

		virtual void onAutoSize();

		virtual void onSizeChange();

		virtual void onPaddingChange();

		void fixSliderPos();

		void adjustSliderPos();

		virtual Uint32 onKeyDown( const KeyEvent &Event );

		virtual void onAlphaChange();

		virtual Uint32 onMessage( const NodeMessage * Msg );
};

}}

#endif


