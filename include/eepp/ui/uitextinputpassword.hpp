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

		virtual UITextBox * setText( const String& text );

		TextCache * getPassCache() const;

		void setFontStyleConfig( const TooltipStyleConfig& fontStyleConfig );
	protected:
		TextCache *	mPassCache;

		void alignFix();

		void autoAlign();

		void updateText();

		void updatePass( const String& pass );
};

}}

#endif
