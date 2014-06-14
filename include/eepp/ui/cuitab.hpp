#ifndef EE_UICUITAB_HPP
#define EE_UICUITAB_HPP

#include <eepp/ui/cuiselectbutton.hpp>

namespace EE { namespace UI {

class cUITabWidget;

class EE_API cUITab : public cUISelectButton {
	public:
		cUITab( cUISelectButton::CreateParams& Params, cUIControl * CtrlOwned );

		cUIControl * CtrlOwned() const;

		virtual ~cUITab();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		virtual const String& Text();

		virtual void Text( const String& text );

		virtual void Update();
	protected:
		cUIControl *	mCtrlOwned;

		virtual Uint32 OnMouseClick( const Vector2i &Pos, const Uint32 Flags );

		virtual void OnStateChange();

		void SetRealSize();

		cUITabWidget * GetTabWidget();
};

}}

#endif
