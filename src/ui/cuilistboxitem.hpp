#ifndef EE_UICUILISTBOXITEM_HPP
#define EE_UICUILISTBOXITEM_HPP

#include "cuitextbox.hpp"

namespace EE { namespace UI {

class EE_API cUIListBoxItem : public cUITextBox {
	public:
		cUIListBoxItem( cUITextBox::CreateParams& Params );

		virtual ~cUIListBoxItem();

		virtual void SetTheme( cUITheme * Theme );

		virtual Uint32 OnMouseClick( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseExit( const eeVector2i& Pos, const Uint32 Flags );

		virtual void Update();

		const bool& Selected() const;

		void Unselect();
	protected:
		bool mSelected;

		virtual void OnStateChange();
};

}}

#endif

