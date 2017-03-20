#ifndef EE_UICUITEXTINPUTPASSWORD_HPP
#define EE_UICUITEXTINPUTPASSWORD_HPP

#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class UITextInputPassword : public UITextInput
{
	public:
		static UITextInputPassword * New();

		UITextInputPassword();

		~UITextInputPassword();

		virtual void draw();

		virtual const String& getText();

		virtual UITextView * setText( const String& text );

		Text * getPassCache() const;

		void setFontStyleConfig( const UITooltipStyleConfig& fontStyleConfig );
	protected:
		Text *	mPassCache;

		void alignFix();

		void updateText();

		void updatePass( const String& pass );
};

}}

#endif
