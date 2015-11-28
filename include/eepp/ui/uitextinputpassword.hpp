#ifndef EE_UICUITEXTINPUTPASSWORD_HPP
#define EE_UICUITEXTINPUTPASSWORD_HPP

#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class UITextInputPassword : public UITextInput
{
	public:
		UITextInputPassword( const UITextInput::CreateParams& Params );

		virtual void Draw();

		virtual const String& Text();

		virtual void Text( const String& text );

		TextCache * GetPassCache() const;
	protected:
		TextCache *	mPassCache;

		void AlignFix();

		void AutoAlign();

		void UpdateText();

		void UpdatePass( const String& pass );
};

}}

#endif
