#ifndef EE_UICUITEXTINPUTPASSWORD_HPP
#define EE_UICUITEXTINPUTPASSWORD_HPP

#include <eepp/ui/cuitextinput.hpp>

namespace EE { namespace UI {

class cUITextInputPassword : public cUITextInput
{
	public:
		cUITextInputPassword( const cUITextInput::CreateParams& Params );

		virtual void Draw();

		virtual const String& Text();

		virtual void Text( const String& text );

		cTextCache * GetPassCache() const;
	protected:
		cTextCache *	mPassCache;

		void AlignFix();

		void AutoAlign();

		void UpdateText();

		void UpdatePass( const String& pass );
};

}}

#endif
