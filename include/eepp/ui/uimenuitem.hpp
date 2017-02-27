#ifndef EE_UICUIMENUITEM_HPP
#define EE_UICUIMENUITEM_HPP

#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UIMenuItem : public UIPushButton {
	public:
		UIMenuItem( UIMenuItem::CreateParams& Params );

		UIMenuItem();

		virtual ~UIMenuItem();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );
	protected:
		virtual Uint32 onMouseEnter( const Vector2i &position, const Uint32 flags );

		virtual void onStateChange();
};

}}

#endif
