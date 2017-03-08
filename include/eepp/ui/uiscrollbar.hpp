#ifndef EE_UICUISCROLLBAR_HPP
#define EE_UICUISCROLLBAR_HPP

#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uislider.hpp>

namespace EE { namespace UI {

class EE_API UIScrollBar : public UIWidget {
	public:
		static UIScrollBar * New();

		UIScrollBar();

		virtual ~UIScrollBar();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setValue( Float Val );

		const Float& getValue() const;

		virtual void setMinValue( const Float& MinVal );

		const Float& getMinValue() const;

		virtual void setMaxValue( const Float& MaxVal );

		const Float& getMaxValue() const;

		virtual void setClickStep( const Float& step );

		const Float& getClickStep() const;

		Float getPageStep() const;

		void setPageStep( const Float& pageStep );

		virtual void setTheme( UITheme * Theme );

		bool isVertical() const;

		virtual void update();

		UISlider * getSlider() const;

		UIControlAnim * getButtonUp() const;

		UIControlAnim * getButtonDown() const;

		UI_ORIENTATION getOrientation() const;

		UIControl * setOrientation( const UI_ORIENTATION & orientation );

		bool getExpandBackground() const;

		void setExpandBackground( bool expandBackground );
	protected:
		UISlider * 		mSlider;
		UIControlAnim *	mBtnUp;
		UIControlAnim * mBtnDown;

		virtual void onSizeChange();

		virtual void onAutoSize();

		void adjustChilds();

		void onValueChangeCb( const UIEvent * Event );

		virtual void onAlphaChange();

		virtual Uint32 onMessage( const UIMessage * Msg );

		void manageClick( const Uint32& flags );
};

}}

#endif

