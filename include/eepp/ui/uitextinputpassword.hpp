#ifndef EE_UICUITEXTINPUTPASSWORD_HPP
#define EE_UICUITEXTINPUTPASSWORD_HPP

#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class UITextInputPassword : public UITextInput
{
	public:
		UITextInputPassword( const UITextInput::CreateParams& Params );

		virtual void draw();

		virtual const String& text();

		virtual void text( const String& text );

		TextCache * getPassCache() const;
	protected:
		TextCache *	mPassCache;

		void alignFix();

		void autoAlign();

		void updateText();

		void updatePass( const String& pass );
};

}}

#endif
