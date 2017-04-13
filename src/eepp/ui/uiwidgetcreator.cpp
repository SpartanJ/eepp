#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiselectbutton.hpp>
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
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uiimage.hpp>

namespace  EE { namespace UI {

typedef std::map<std::string, UIWidgetCreator::CustomWidgetCb> widgetCallbackMap;

static widgetCallbackMap widgetCallback;

UIWidget * UIWidgetCreator::createFromName( std::string widgetName ) {
	String::toLowerInPlace( widgetName );

	if ( widgetName == "widget" )				return UIWidget::New();
	else if ( widgetName == "horizontallinearlayout" || widgetName == "hll" )									return UILinearLayout::NewHorizontal();
	else if ( widgetName == "linearlayout" || widgetName == "verticallinearlayout" || widgetName == "vll" )		return UILinearLayout::NewVertical();
	else if ( widgetName == "relativelayout" )	return UIRelativeLayout::New();
	else if ( widgetName == "textview" )		return UITextView::New();
	else if ( widgetName == "pushbutton" )		return UIPushButton::New();
	else if ( widgetName == "checkbox" )		return UICheckBox::New();
	else if ( widgetName == "radiobutton" )		return UIRadioButton::New();
	else if ( widgetName == "combobox" )		return UIComboBox::New();
	else if ( widgetName == "dropdownlist" )	return UIDropDownList::New();
	else if ( widgetName == "image" )			return UIImage::New();
	else if ( widgetName == "listbox" )			return UIListBox::New();
	else if ( widgetName == "winmenu" )			return UIWinMenu::New();
	else if ( widgetName == "progressbar" )		return UIProgressBar::New();
	else if ( widgetName == "scrollbar" )		return UIScrollBar::New();
	else if ( widgetName == "slider" )			return UISlider::New();
	else if ( widgetName == "spinbox" )			return UISpinBox::New();
	else if ( widgetName == "sprite" )			return UISprite::New();
	else if ( widgetName == "tab" )				return UITab::New();
	else if ( widgetName == "table" )			return UITable::New();
	else if ( widgetName == "tablecell" )		return UITableCell::New();
	else if ( widgetName == "tabwidget" )		return UITabWidget::New();
	else if ( widgetName == "textedit" )		return UITextEdit::New();
	else if ( widgetName == "textinput" || widgetName == "input" )						return UITextInput::New();
	else if ( widgetName == "textinputpassword" || widgetName == "inputpassword" )		return UITextInputPassword::New();
	else if ( widgetName == "loader" )			return UILoader::New();
	else if ( widgetName == "selectbutton" )	return UISelectButton::New();
	else if ( widgetName == "window" )			return UIWindow::New();
	else if ( widgetName == "scrollview" )		return UIScrollView::New();
	else if ( widgetName == "subtexture" )		return UISubTexture::New();

	if ( widgetCallback.find( widgetName ) != widgetCallback.end() ) {
		return widgetCallback[ widgetName ].Call( widgetName );
	}

	return NULL;
}

void UIWidgetCreator::addCustomWidgetCallback( std::string widgetName, const UIWidgetCreator::CustomWidgetCb& cb ) {
	widgetCallback[ String::toLower( widgetName ) ] = cb;
}

void UIWidgetCreator::removeCustomWidgetCallback( std::string widgetName ) {
	widgetCallback.erase( String::toLower( widgetName ) );
}

bool UIWidgetCreator::existsCustomWidgetCallback( std::string widgetName ) {
	return widgetCallback.find( String::toLower( widgetName ) ) != widgetCallback.end();
}

}}
