#include <eepp/ui/tools/uitextureviewer.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uiconsole.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uigridlayout.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/uisprite.hpp>
#include <eepp/ui/uistacklayout.hpp>
#include <eepp/ui/uistackwidget.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextinputpassword.hpp>
#include <eepp/ui/uitextureregion.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uitouchdraggablewidget.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiviewpager.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/ui/uiwidgettable.hpp>
#include <eepp/ui/uiwidgettablerow.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI {

static bool sBaseListCreated = false;

UIWidgetCreator::WidgetCallbackMap UIWidgetCreator::widgetCallback =
	UIWidgetCreator::WidgetCallbackMap();

UIWidgetCreator::RegisteredWidgetCallbackMap UIWidgetCreator::registeredWidget =
	UIWidgetCreator::RegisteredWidgetCallbackMap();

void UIWidgetCreator::createBaseWidgetList() {
	if ( !sBaseListCreated ) {
		registeredWidget["widget"] = UIWidget::New;
		registeredWidget["linearlayout"] = UILinearLayout::NewVertical;
		registeredWidget["relativelayout"] = UIRelativeLayout::New;
		registeredWidget["textview"] = UITextView::New;
		registeredWidget["pushbutton"] = UIPushButton::New;
		registeredWidget["checkbox"] = UICheckBox::New;
		registeredWidget["radiobutton"] = UIRadioButton::New;
		registeredWidget["combobox"] = UIComboBox::New;
		registeredWidget["dropdownlist"] = UIDropDownList::New;
		registeredWidget["image"] = UIImage::New;
		registeredWidget["listbox"] = UIListBox::New;
		registeredWidget["menubar"] = UIMenuBar::New;
		registeredWidget["progressbar"] = UIProgressBar::New;
		registeredWidget["scrollbar"] = UIScrollBar::New;
		registeredWidget["slider"] = UISlider::New;
		registeredWidget["spinbox"] = UISpinBox::New;
		registeredWidget["sprite"] = UISprite::New;
		registeredWidget["tab"] = UITab::New;
		registeredWidget["widgettable"] = UIWidgetTable::New;
		registeredWidget["widgettablerow"] = UIWidgetTableRow::New;
		registeredWidget["tabwidget"] = UITabWidget::New;
		registeredWidget["textedit"] = UITextEdit::New;
		registeredWidget["textinput"] = UITextInput::New;
		registeredWidget["textinputpassword"] = UITextInputPassword::New;
		registeredWidget["loader"] = UILoader::New;
		registeredWidget["selectbutton"] = UISelectButton::New;
		registeredWidget["window"] = UIWindow::New;
		registeredWidget["scrollview"] = UIScrollView::New;
		registeredWidget["textureregion"] = UITextureRegion::New;
		registeredWidget["touchdraggable"] = UITouchDraggableWidget::New;
		registeredWidget["gridlayout"] = UIGridLayout::New;
		registeredWidget["stacklayout"] = UIStackLayout::New;
		registeredWidget["viewpager"] = UIViewPager::New;
		registeredWidget["codeeditor"] = UICodeEditor::New;
		registeredWidget["splitter"] = UISplitter::New;
		registeredWidget["treeview"] = UITreeView::New;
		registeredWidget["tableview"] = UITableView::New;
		registeredWidget["listview"] = UIListView::New;
		registeredWidget["stackwidget"] = UIStackWidget::New;
		registeredWidget["console"] = UIConsole::New;
		// registeredWidget["menu"] = UIMenu::New;
		registeredWidget["menucheckbox"] = UIMenuCheckBox::New;
		registeredWidget["menuradiobutton"] = UIMenuRadioButton::New;
		registeredWidget["menuseparator"] = UIMenuSeparator::New;
		registeredWidget["anchor"] = UIAnchor::New;
		registeredWidget["textureviewer"] = Tools::UITextureViewer::New;

		registeredWidget["hbox"] = UILinearLayout::NewHorizontal;
		registeredWidget["vbox"] = UILinearLayout::NewVertical;
		registeredWidget["input"] = UITextInput::New;
		registeredWidget["inputpassword"] = UITextInputPassword::New;
		registeredWidget["viewpagerhorizontal"] = UIViewPager::NewHorizontal;
		registeredWidget["viewpagervertical"] = UIViewPager::NewHorizontal;
		registeredWidget["vslider"] = UISlider::NewHorizontal;
		registeredWidget["hslider"] = UISlider::NewHorizontal;
		registeredWidget["vscrollbar"] = UIScrollBar::NewVertical;
		registeredWidget["hscrollbar"] = UIScrollBar::NewHorizontal;
		registeredWidget["button"] = UIPushButton::New;
		registeredWidget["rlay"] = UIRelativeLayout::New;
		registeredWidget["tooltip"] = UITooltip::New;
		registeredWidget["tv"] = UITextView::New;
		registeredWidget["a"] = UIAnchor::New;

		sBaseListCreated = true;
	}
}

UIWidget* UIWidgetCreator::createFromName( const std::string& widgetName ) {
	createBaseWidgetList();

	std::string lwidgetName( String::toLower( widgetName ) );

	if ( registeredWidget.find( lwidgetName ) != registeredWidget.end() ) {
		return registeredWidget[lwidgetName]();
	}

	if ( widgetCallback.find( lwidgetName ) != widgetCallback.end() ) {
		return widgetCallback[lwidgetName]( lwidgetName );
	}

	return NULL;
}

void UIWidgetCreator::addCustomWidgetCallback( const std::string& widgetName,
											   const UIWidgetCreator::CustomWidgetCb& cb ) {
	widgetCallback[String::toLower( widgetName )] = cb;
}

void UIWidgetCreator::removeCustomWidgetCallback( const std::string& widgetName ) {
	widgetCallback.erase( String::toLower( widgetName ) );
}

bool UIWidgetCreator::existsCustomWidgetCallback( const std::string& widgetName ) {
	return widgetCallback.find( String::toLower( widgetName ) ) != widgetCallback.end();
}

void UIWidgetCreator::registerWidget( const std::string& widgetName,
									  const UIWidgetCreator::RegisterWidgetCb& cb ) {
	registeredWidget[String::toLower( widgetName )] = cb;
}

void UIWidgetCreator::unregisterWidget( const std::string& widgetName ) {
	registeredWidget.erase( String::toLower( widgetName ) );
}

bool UIWidgetCreator::isWidgetRegistered( const std::string& widgetName ) {
	return registeredWidget.find( String::toLower( widgetName ) ) != registeredWidget.end();
}

const UIWidgetCreator::RegisteredWidgetCallbackMap& UIWidgetCreator::getRegisteredWidgets() {
	return registeredWidget;
}

std::vector<std::string> UIWidgetCreator::getWidgetNames() {
	std::vector<std::string> names;
	names.reserve( registeredWidget.size() );
	createBaseWidgetList();
	for ( const auto& widgetIt : registeredWidget )
		names.push_back( widgetIt.first );
	return names;
}

}} // namespace EE::UI
