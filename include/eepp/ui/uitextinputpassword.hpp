#ifndef EE_UICUITEXTINPUTPASSWORD_HPP
#define EE_UICUITEXTINPUTPASSWORD_HPP

#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class UITextInputPassword : public UITextInput
{
	public:
		UITextInputPassword( const UITextInput::CreateParams& Params );

		UITextInputPassword();

		virtual void draw();

		virtual const String& getText();

		virtual void setText( const String& text );

		TextCache * getPassCache() const;

		void setFontStyleConfig( const FontStyleConfig& fontStyleConfig );
	protected:
		TextCache *	mPassCache;

		void alignFix();

		void autoAlign();

		void updateText();

		void updatePass( const String& pass );
};

}}

#endif
