#ifndef EE_UICUISPINBOX_HPP
#define EE_UICUISPINBOX_HPP

#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UISpinBox : public UIWidget {
	public:
		static UISpinBox * New();

		UISpinBox();

		virtual ~UISpinBox();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual void setPadding( const Recti& padding );

		const Recti& getPadding() const;

		virtual void setClickStep( const Float& step );

		const Float& getClickStep() const;

		virtual Uint32 onMessage( const UIMessage * Msg );

		void addValue( const Float& value );

		virtual UISpinBox * setMinValue( const Float& MinVal );

		const Float& getMinValue() const;

		virtual UISpinBox * setMaxValue( const Float& MaxVal );

		const Float& getMaxValue() const;

		virtual UISpinBox * setValue( const Float& Val );

		const Float& getValue() const;

		virtual void update();

		UIControlAnim * getButtonPushUp() const;

		UIControlAnim * getButtonPushDown() const;

		UITextInput * getTextInput() const;

		UISpinBox * setAllowOnlyNumbers( bool allow );

		bool dotsInNumbersAllowed();

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		UITextInput * 		mInput;
		UIControlAnim * 	mPushUp;
		UIControlAnim * 	mPushDown;
		Float				mMinValue;
		Float				mMaxValue;
		Float				mValue;
		Float				mClickStep;

		void adjustChilds();

		void internalValue( const Float& Val, const bool& Force = false );

		virtual void onSizeChange();
		
		virtual void onAlphaChange();
};

}}

#endif
