#ifndef EE_UICUICOMBOBOX_HPP
#define EE_UICUICOMBOBOX_HPP

#include <eepp/ui/cuidropdownlist.hpp>

namespace EE { namespace UI {

class EE_API cUIComboBox : public cUIDropDownList {
	public:
		cUIComboBox( cUIComboBox::CreateParams& Params );

		virtual ~cUIComboBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );
	protected:
		cUIControl * mButton;

		void OnButtonClick( const cUIEvent * Event );

		void OnButtonEnter( const cUIEvent * Event );

		void OnButtonExit( const cUIEvent * Event );

		Uint32 OnMouseClick( const Vector2i& Pos, const Uint32 Flags );

		void CreateButton();

		virtual void OnControlClear( const cUIEvent *Event );
};

}}

#endif
