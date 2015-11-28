#ifndef EE_UICUICOMBOBOX_HPP
#define EE_UICUICOMBOBOX_HPP

#include <eepp/ui/uidropdownlist.hpp>

namespace EE { namespace UI {

class EE_API UIComboBox : public UIDropDownList {
	public:
		UIComboBox( UIComboBox::CreateParams& Params );

		virtual ~UIComboBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );
	protected:
		UIControl * mButton;

		void OnButtonClick( const UIEvent * Event );

		void OnButtonEnter( const UIEvent * Event );

		void OnButtonExit( const UIEvent * Event );

		Uint32 OnMouseClick( const Vector2i& Pos, const Uint32 Flags );

		void CreateButton();

		virtual void OnControlClear( const UIEvent *Event );
};

}}

#endif
