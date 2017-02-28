#ifndef EE_UICUICOMBOBOX_HPP
#define EE_UICUICOMBOBOX_HPP

#include <eepp/ui/uidropdownlist.hpp>

namespace EE { namespace UI {

class EE_API UIComboBox : public UIComplexControl {
	public:
		static UIComboBox * New();

		UIComboBox();

		virtual ~UIComboBox();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		UIListBox * getListBox();

		UIDropDownList * getDropDownList() const { return mDropDownList; }

		UIComplexControl * getButton() const { return mButton; }

		InputTextBuffer * getInputTextBuffer();

		const String& getText();
	protected:
		UIDropDownList * mDropDownList;
		UIComplexControl * mButton;

		void onButtonClick( const UIEvent * Event );

		void onButtonEnter( const UIEvent * Event );

		void onButtonExit( const UIEvent * Event );

		void updateControls();

		virtual void onSizeChange();

		virtual void onPositionChange();
};

}}

#endif
