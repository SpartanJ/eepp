#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uisubtexture.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uisprite.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitable.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextinputpassword.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace  EE { namespace UI {

static UIHelper::CreateUIWidgetCb customWidgetCallback;

UIWidget * UIHelper::createUIWidgetFromName( std::string name ) {
	String::toLowerInPlace( name );

	if ( name == "widget" ) {
		return UIWidget::New();
	} else if ( name == "horizontallinearlayout" || name == "hll" ) {
		return UILinearLayout::NewHorizontal();
	} else if ( name == "linearlayout" || name == "verticallinearlayout" || name == "vll" ) {
		return UILinearLayout::NewVertical();
	} else if ( name == "relativelayout" ) {
		return UIRelativeLayout::New();
	} else if ( name == "textview" ) {
		return UITextView::New();
	} else if ( name == "pushbutton" ) {
		return UIPushButton::New();
	} else if ( name == "checkbox" ) {
		return UICheckBox::New();
	} else if ( name == "radiobutton" ) {
		return UIRadioButton::New();
	} else if ( name == "combobox" ) {
		return UIComboBox::New();
	} else if ( name == "dropdownlist" ) {
		return UIDropDownList::New();
	} else if ( name == "image" ) {
		return UISubTexture::New();
	} else if ( name == "listbox" ) {
		return UIListBox::New();
	} else if ( name == "winmenu" ) {
		return UIWinMenu::New();
	} else if ( name == "progressbar" ) {
		return UIProgressBar::New();
	} else if ( name == "scrollbar" ) {
		return UIScrollBar::New();
	} else if ( name == "slider" ) {
		return UISlider::New();
	} else if ( name == "spinbox" ) {
		return UISpinBox::New();
	} else if (  name == "sprite" ) {
		return UISprite::New();
	} else if ( name == "tab" ) {
		return UITab::New();
	} else if ( name == "table" ) {
		return UITable::New();
	} else if ( name == "tablecell" ) {
		return UITableCell::New();
	} else if ( name == "tabwidget" ) {
		return UITabWidget::New();
	} else if ( name == "textedit" ) {
		return UITextEdit::New();
	} else if ( name == "textinput" || name == "input" ) {
		return UITextInput::New();
	} else if ( name == "textinputpassword" || name == "inputpassword" ) {
		return UITextInputPassword::New();
	} else if ( name == "loader" ) {
		return UILoader::New();
	} else if ( name == "window" ) {
		return UIWindow::New();
	}

	if ( customWidgetCallback.IsSet() ) {
		return customWidgetCallback( name );
	}

	return NULL;
}

void UIHelper::setCreateCustomUIWidgetCallback(UIHelper::CreateUIWidgetCb cb) {
	customWidgetCallback = cb;
}

}}
