#ifndef EE_UICUITAB_HPP
#define EE_UICUITAB_HPP

#include <eepp/ui/uiselectbutton.hpp>

namespace EE { namespace UI {

class UITabWidget;

class EE_API UITab : public UISelectButton {
	public:
		UITab( UISelectButton::CreateParams& Params, UIControl * CtrlOwned );

		UIControl * CtrlOwned() const;

		virtual ~UITab();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		virtual const String& Text();

		virtual void Text( const String& text );

		virtual void Update();
	protected:
		UIControl *	mCtrlOwned;

		virtual Uint32 OnMouseClick( const Vector2i &Pos, const Uint32 Flags );

		virtual void OnStateChange();

		void SetRealSize();

		UITabWidget * GetTabWidget();
};

}}

#endif
