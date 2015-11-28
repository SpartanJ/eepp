#ifndef EE_UICUISELECTBUTTON_HPP
#define EE_UICUISELECTBUTTON_HPP

#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UISelectButton : public UIPushButton {
	public:
		UISelectButton( const UIPushButton::CreateParams& Params );

		virtual ~UISelectButton();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual bool Selected() const;

		virtual void Unselect();

		virtual void Select();
	protected:
		virtual void OnStateChange();
};

}}

#endif
