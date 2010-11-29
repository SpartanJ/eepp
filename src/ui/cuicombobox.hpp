#ifndef EE_UICUICOMBOBOX_HPP
#define EE_UICUICOMBOBOX_HPP

#include "cuidropdownlist.hpp"

namespace EE { namespace UI {

class cUIComboBox : public cUIDropDownList {
	public:
		cUIComboBox( cUIComboBox::CreateParams& Params );

		~cUIComboBox();

		virtual void SetTheme( cUITheme * Theme );

		Uint32 OnMouseClick( const eeVector2i& Pos, const Uint32 Flags );
	protected:
		virtual void OnItemSelected( const cUIEvent * Event );
};

}}

#endif

