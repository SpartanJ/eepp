#ifndef EE_UICUISELECTBUTTON_HPP
#define EE_UICUISELECTBUTTON_HPP

#include "cuipushbutton.hpp"

namespace EE { namespace UI {

class EE_API cUISelectButton : public cUIPushButton {
	public:
		cUISelectButton( const cUIPushButton::CreateParams& Params );

		virtual ~cUISelectButton();

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
