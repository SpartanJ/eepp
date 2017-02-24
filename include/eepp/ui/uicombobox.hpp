#ifndef EE_UICUICOMBOBOX_HPP
#define EE_UICUICOMBOBOX_HPP

#include <eepp/ui/uidropdownlist.hpp>

namespace EE { namespace UI {

class EE_API UIComboBox : public UIDropDownList {
	public:
		UIComboBox( UIComboBox::CreateParams& Params );

		UIComboBox();

		virtual ~UIComboBox();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );
	protected:
		UIControl * mButton;

		void onButtonClick( const UIEvent * Event );

		void onButtonEnter( const UIEvent * Event );

		void onButtonExit( const UIEvent * Event );

		Uint32 onMouseClick( const Vector2i& position, const Uint32 flags );

		void createButton();

		virtual void onControlClear( const UIEvent *Event );

		virtual void onSizeChange();
};

}}

#endif
