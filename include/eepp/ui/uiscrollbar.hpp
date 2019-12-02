#ifndef EE_UICUISCROLLBAR_HPP
#define EE_UICUISCROLLBAR_HPP

#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uislider.hpp>

namespace EE { namespace UI {

class EE_API UIScrollBar : public UIWidget {
	public:
		enum ScrollBarType {
			TwoButtons,
			NoButtons
		};

		static UIScrollBar * New();

		static UIScrollBar * NewHorizontal();

		static UIScrollBar * NewVertical();

		explicit UIScrollBar( const UI_ORIENTATION& orientation = UI_VERTICAL );

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

		UISlider * getSlider() const;

		UINode * getButtonUp() const;

		UINode * getButtonDown() const;

		UI_ORIENTATION getOrientation() const;

		UINode * setOrientation( const UI_ORIENTATION & orientation );

		ScrollBarType getScrollBarType() const;

		void setScrollBarType(const ScrollBarType & scrollBarType);

		bool getExpandBackground() const;

		void setExpandBackground( bool expandBackground );

		virtual bool applyProperty( const StyleSheetProperty& attribute );

		virtual std::string getPropertyString(const PropertyDefinition* propertyDef);
	protected:
		ScrollBarType	mScrollBarType;
		UISlider * 		mSlider;
		UINode *	mBtnUp;
		UINode * mBtnDown;

		virtual void onSizeChange();

		virtual void onAutoSize();

		void adjustChilds();

		void onValueChangeCb( const Event * Event );

		virtual void onAlphaChange();

		virtual Uint32 onMessage( const NodeMessage * Msg );

		virtual void onPaddingChange();
};

}}

#endif

