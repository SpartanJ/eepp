#ifndef EE_UICUICOMBOBOX_HPP
#define EE_UICUICOMBOBOX_HPP

#include "cuidropdownlist.hpp"

namespace EE { namespace UI {

class EE_API cUIComboBox : public cUIDropDownList {
	public:
		cUIComboBox( cUIComboBox::CreateParams& Params );

		~cUIComboBox();

		virtual void SetTheme( cUITheme * Theme );

		Uint32 OnMouseClick( const eeVector2i& Pos, const Uint32 Flags );
	protected:
		cUIControl * mButton;

		virtual void OnItemSelected( const cUIEvent * Event );

		void OnButtonClick( const cUIEvent * Event );

		void CreateButton();
};

}}

#endif

