#ifndef EE_UICUIMENUITEM_HPP
#define EE_UICUIMENUITEM_HPP

#include "cuipushbutton.hpp"

namespace EE { namespace UI {

class EE_API cUIMenuItem : public cUIPushButton {
	public:
		cUIMenuItem( cUIMenuItem::CreateParams& Params );

		virtual ~cUIMenuItem();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );
	protected:
		virtual Uint32 OnMouseEnter( const eeVector2i &Pos, const Uint32 Flags );

		virtual void OnStateChange();
};

}}

#endif
