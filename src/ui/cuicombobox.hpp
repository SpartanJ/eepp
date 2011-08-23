#ifndef EE_UICUICOMBOBOX_HPP
#define EE_UICUICOMBOBOX_HPP

#include "cuidropdownlist.hpp"

namespace EE { namespace UI {

class EE_API cUIComboBox : public cUIDropDownList {
	public:
		cUIComboBox( cUIComboBox::CreateParams& Params );

		~cUIComboBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );
	protected:
		cUIControl * mButton;

		void OnButtonClick( const cUIEvent * Event );

		Uint32 OnMouseClick( const eeVector2i& Pos, const Uint32 Flags );

		void CreateButton();

		virtual void OnControlClear( const cUIEvent *Event );
};

}}

#endif

