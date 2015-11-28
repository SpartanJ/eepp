#ifndef EE_UICUIMENUITEM_HPP
#define EE_UICUIMENUITEM_HPP

#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UIMenuItem : public UIPushButton {
	public:
		UIMenuItem( UIMenuItem::CreateParams& Params );

		virtual ~UIMenuItem();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );
	protected:
		virtual Uint32 OnMouseEnter( const Vector2i &Pos, const Uint32 Flags );

		virtual void OnStateChange();
};

}}

#endif
