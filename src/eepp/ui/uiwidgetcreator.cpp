#include <eepp/ui/charts/uichart.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/tools/uidiffview.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/tools/uimergeview.hpp>
#include <eepp/ui/tools/uitextureviewer.hpp>
#include <eepp/ui/uicalendar.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uiconsole.hpp>
#include <eepp/ui/uidatepicker.hpp>
#include <eepp/ui/uidatetimeedit.hpp>
#include <eepp/ui/uidatetimepicker.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uiflowlayout.hpp>
#include <eepp/ui/uigridlayout.hpp>
#include <eepp/ui/uihtmldetails.hpp>
#include <eepp/ui/uihtmlform.hpp>
#include <eepp/ui/uihtmlimage.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uihtmllistitem.hpp>
#include <eepp/ui/uihtmltable.hpp>
#include <eepp/ui/uihtmltextarea.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uimarkdownview.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uinodelink.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/uisprite.hpp>
#include <eepp/ui/uistackwidget.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uisvg.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uitextureregion.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitimepicker.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uitouchdraggablewidget.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiviewpager.hpp>
#include <eepp/ui/uiwebview.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/ui/uiwidgettable.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI {

static bool sBaseListCreated = false;

UIWidgetCreator::WidgetCallbackMap UIWidgetCreator::widgetCallback =
	UIWidgetCreator::WidgetCallbackMap();

UIWidgetCreator::RegisteredWidgetCallbackMap UIWidgetCreator::registeredWidget =
	UIWidgetCreator::RegisteredWidgetCallbackMap();

static const std::string_view getHTMLBasicDefaultsCSS() {
	return R"css(
body { margin: 0.67em; }

h1 { font-size: 2em; font-weight: bold; margin: 0.67em 0; }
h2 { font-size: 1.5em; font-weight: bold; margin: 0.83em 0; }
h3 { font-size: 1.17em; font-weight: bold; margin: 1em 0; }
h4 { font-size: 1em; font-weight: bold; margin: 1.33em 0; }
h5 { font-size: 0.83em; font-weight: bold; margin: 1.67em 0; }
h6 { font-size: 0.67em; font-weight: bold; margin: 2.33em 0; }

p { margin: 1em 0; }
pre { margin: 1em 0; white-space: pre; }
blockquote { margin: 1em 0; }
hr { display: block; border-width: 1px; border-style: solid; margin: 0.5em 0; min-height: 2px; }
ul, ol, dl { margin: 1em 0; }

b, strong { font-weight: bold; }
i, em, cite { font-style: italic; }
small { font-size: smaller; }
u, ins { text-decoration: underline; }
s, strike, del { text-decoration: line-through; }
code, kbd { font-family: monospace; }
sub, sup { font-size: smaller; }
mark { background-color: yellow; }

a, a:link { text-decoration: none; cursor: arrow; }
a:hover { text-decoration: underline; cursor: hand; }

ul { padding-left: 40dp; list-style-type: disc; }
ol { padding-left: 40dp; list-style-type: decimal; }
dd { margin-left: 40dp; }

summary { cursor: pointer; padding-left: 20dp; list-style-type: disclosure-closed; }

form { margin-top: 0em; margin-bottom: 1em; }
)css";
}

static const std::string_view getHTMLDocumentDefaultsCSS() {
	return R"css(
body { color: black; }
mark { background-color: yellow; }

hr { border-color: gray; }
a, a:link { color: #0000EE; }
a:visited { color: #551A8B; }

textarea { border-width: 1dp; border-style: solid; border-color: #767676; background-color: white; color: black; padding: 2dp; selection-back-color: lightgray; }

input[type="text"],
input[type="password"],
input[type="number"],
input[type="date"],
input[type="time"],
input[type="datetime-local"] {
	border-width: 1dp;
	border-style: solid;
	border-color: #767676;
	background-color: white;
	color: black;
	padding-top: 1dp;
	padding-bottom: 1dp;
	padding-left: 2dp;
	padding-right: 2dp;
	hint-color: #767676;
}

button,
select,
meter,
progress,
input[type="submit"],
input[type="button"],
input[type="image"],
input[type="reset"] { user-select: none; }

button,
input[type="submit"],
input[type="button"],
input[type="reset"] {
	display: inline-block;
	box-sizing: border-box;
	border-width: 1dp;
	border-style: solid;
	border-color: #767676;
	background-color: #f0f0f0;
	color: black;
	padding-top: 2dp;
	padding-bottom: 3dp;
	padding-left: 6dp;
	padding-right: 6dp;
	text-decoration: none;
}

button:hover,
input[type="submit"]:hover,
input[type="button"]:hover,
input[type="reset"]:hover {
	background-color: #e5e5e5;
}

input[type="checkbox"],
input[type="radio"] {
	text-decoration: none;
}

CheckBox::active,
CheckBox::inactive {
	width: 12dp;
	height: 12dp;
	border-width: 1dp;
}

CheckBox::inactive {
	border-color: #767676;
}

CheckBox::active {
	border-color: black;
	foreground-image: rectangle(solid, black);
	foreground-size: 8dpru 8dpru;
	foreground-position: center;
}

RadioButton::active,
RadioButton::inactive {
	width: 12dp;
	height: 12dp;
	border-width: 1dp;
	border-radius: 100%;
}

RadioButton::inactive {
	border-color: #767676;
}

RadioButton::active {
	border-color: black;
	foreground-image: circle(solid, black);
	foreground-size: 8dp 8dp;
	foreground-position: 6dp 6dp;
}

/* Anonymous temporal controls use browser-like light defaults within the document. */
Calendar {
	color: black;
	font-size: 12dp;
	background-color: white;
	border-color: #767676;
	border-width: 1dp;
	width: auto;
	height: auto;
	min-width: 160dp;
	min-height: 160dp;
}
Calendar::previous, Calendar::next, Calendar::title, Calendar::today, Calendar::day {
	color: black;
	padding: 0;
	border-width: 0;
	background-color: transparent;
}
Calendar::previous {
	foreground-image: url("data:image/svg,<svg viewBox='0 0 24 24' fill='white'><path d='M10.8284 12.0007L15.7782 16.9504L14.364 18.3646L8 12.0007L14.364 5.63672L15.7782 7.05093L10.8284 12.0007Z'/></svg>");
}
Calendar::next {
	foreground-image: url("data:image/svg,<svg viewBox='0 0 24 24' fill='white'><path d='M13.1717 12.0007L8.22192 7.05093L9.63614 5.63672L16.0001 12.0007L9.63614 18.3646L8.22192 16.9504L13.1717 12.0007Z'/></svg>");
}
Calendar::previous, Calendar::next {
	cursor: arrow;
	foreground-size: 12dp 12dp;
	foreground-position: center;
	foreground-repeat: no-repeat;
	foreground-tint: black;
}
Calendar::weekday, Calendar::day.outside-month { color: #767676; }
Calendar::day.selected, Calendar::day:hover { background-color: #0078d7; color: white; }
Calendar::day.today { border-color: #767676; border-width: 1dp; }
Calendar::day.focused { border-color: black; border-width: 1dp; }
Calendar::day:disabled { opacity: 0.35; }
DatePicker::button, DateTimePicker::button {
	foreground-image: url("data:image/svg,<svg viewBox='0 0 24 24' fill='white'><path d='M9 1V3H15V1H17V3H21C21.5523 3 22 3.44772 22 4V20C22 20.5523 21.5523 21 21 21H3C2.44772 21 2 20.5523 2 20V4C2 3.44772 2.44772 3 3 3H7V1H9ZM20 11H4V19H20V11ZM7 5H4V9H20V5H17V7H15V5H9V7H7V5Z'/></svg>");
	foreground-size: 12dp 12dp;
	foreground-position: center;
	foreground-repeat: no-repeat;
	foreground-tint: black;
	padding: 0;
	cursor: arrow;
	border-width: 0;
	background-color: transparent;
}
DatePicker:disabled > DatePicker::button, DateTimePicker:disabled > DateTimePicker::button,
DatePicker[allow-editing=false] > DatePicker::button,
DateTimePicker[allow-editing=false] > DateTimePicker::button {
	foreground-tint: #767676;
	background-color: transparent;
	cursor: arrow;
}

/* Combined popup: a reusable calendar followed by a segmented time editor. */
DateTimePicker::popup {
	width: auto;
	height: auto;
	background-color: white;
	border-color: #767676;
	border-width: 1dp;
	border-radius: 2dp;
}
DateTimePicker::popup > Calendar {
	border-width: 0;
	background-color: transparent;
}
DateTimePicker::time {
	text-align: center;
	font-size: 18dp;
	padding: 24dp 8dp;
	min-height: 72dp;
	color: black;
	background-color: transparent;
	border-color: #767676;
	border-width: 1dp 0 0 0;
}
DateTimePicker::time-up, DateTimePicker::time-down {
	foreground-size: 12dp 12dp;
	foreground-position: center;
	foreground-repeat: no-repeat;
	foreground-tint: black;
	background-color: transparent;
	border-width: 0;
	padding: 0;
	width: 24dp;
	height: 22dp;
	cursor: arrow;
}
DateTimePicker::time-up {
	foreground-image: url("data:image/svg,<svg viewBox='0 0 24 24' fill='white'><path d='M12 8L18 14L16.5858 15.4142L12 10.8284L7.41421 15.4142L6 14L12 8Z'/></svg>");
}
DateTimePicker::time-down {
	foreground-image: url("data:image/svg,<svg viewBox='0 0 24 24' fill='white'><path d='M12 16L6 10L7.41421 8.58579L12 13.1716L16.5858 8.58579L18 10L12 16Z'/></svg>");
}

)css";
}

void UIWidgetCreator::createBaseWidgetList() {
	if ( !sBaseListCreated ) {
		registeredWidget["widget"] = UIWidget::New;
		registeredWidget["chart"] = Charts::UIChart::New;
		registeredWidget["linearlayout"] = UILinearLayout::NewVertical;
		registeredWidget["relativelayout"] = UIRelativeLayout::New;
		registeredWidget["textview"] = UITextView::New;
		registeredWidget["pushbutton"] = UIPushButton::New;
		registeredWidget["checkbox"] = UICheckBox::New;
		registeredWidget["radiobutton"] = UIRadioButton::New;
		registeredWidget["combobox"] = UIComboBox::New;
		registeredWidget["dropdownlist"] = UIDropDownList::New;
		registeredWidget["dropdownmodellist"] = UIDropDownModelList::New;
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
		registeredWidget["datetimeedit"] = []() -> UIWidget* { return UIDateTimeEdit::New(); };
		registeredWidget["calendar"] = UICalendar::New;
		registeredWidget["datepicker"] = UIDatePicker::New;
		registeredWidget["timepicker"] = UITimePicker::New;
		registeredWidget["datetimepicker"] = UIDateTimePicker::New;
		registeredWidget["loader"] = UILoader::New;
		registeredWidget["selectbutton"] = UISelectButton::New;
		registeredWidget["window"] = UIWindow::New;
		registeredWidget["windowvbox"] = UIWindow::NewVBox;
		registeredWidget["windowhbox"] = UIWindow::NewHBox;
		registeredWidget["windowrellay"] = UIWindow::NewRelLay;
		registeredWidget["scrollview"] = UIScrollView::New;
		registeredWidget["textureregion"] = UITextureRegion::New;
		registeredWidget["touchdraggable"] = UITouchDraggableWidget::New;
		registeredWidget["gridlayout"] = UIGridLayout::New;
		registeredWidget["flowlayout"] = UIFlowLayout::New;
		registeredWidget["viewpager"] = UIViewPager::New;
		registeredWidget["codeeditor"] = UICodeEditor::New;
		registeredWidget["diffview"] = Tools::UIDiffView::New;
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
		registeredWidget["nodelink"] = UINodeLink::New;
		registeredWidget["textureviewer"] = Tools::UITextureViewer::New;
		registeredWidget["imageviewer"] = Tools::UIImageViewer::New;
		registeredWidget["richtext"] = UIRichText::New;
		registeredWidget["textspan"] = UITextSpan::New;
		registeredWidget["markdownview"] = UIMarkdownView::New;
		registeredWidget["scrollablemarkdownview"] = UIScrollableMarkdownView::New;
		registeredWidget["mergeview"] = Tools::UIMergeView::New;

		// Aliases
		registeredWidget["hbox"] = UILinearLayout::NewHorizontal;
		registeredWidget["vbox"] = UILinearLayout::NewVertical;
		registeredWidget["viewpagerhorizontal"] = UIViewPager::NewHorizontal;
		registeredWidget["viewpagervertical"] = UIViewPager::NewHorizontal;
		registeredWidget["vslider"] = UISlider::NewHorizontal;
		registeredWidget["hslider"] = UISlider::NewHorizontal;
		registeredWidget["vscrollbar"] = UIScrollBar::NewVertical;
		registeredWidget["hscrollbar"] = UIScrollBar::NewHorizontal;
		registeredWidget["rlay"] = UIRelativeLayout::New;
		registeredWidget["tooltip"] = UITooltip::New;
		registeredWidget["tv"] = UITextView::New;

		// HTML elements
		registeredWidget["a"] = UIAnchorSpan::New;
		registeredWidget["label"] = UILabelSpan::New;
		registeredWidget["span"] = UITextSpan::New;
		registeredWidget["em"] = UITextSpan::NewEmphasis;
		registeredWidget["b"] = UITextSpan::NewBold;
		registeredWidget["strong"] = UITextSpan::NewStrong;
		registeredWidget["small"] = UITextSpan::NewSmall;
		registeredWidget["i"] = UITextSpan::NewItalics;
		registeredWidget["cite"] = [] { return UITextSpan::NewWithTag( "cite" ); };
		registeredWidget["kbd"] = [] { return UITextSpan::NewWithTag( "kbd" ); };
		registeredWidget["sub"] = [] { return UITextSpan::NewWithTag( "sub" ); };
		registeredWidget["sup"] = [] { return UITextSpan::NewWithTag( "sup" ); };
		registeredWidget["time"] = [] { return UITextSpan::NewWithTag( "time" ); };
		registeredWidget["u"] = UITextSpan::NewUnderline;
		registeredWidget["ins"] = UITextSpan::NewUnderline;
		registeredWidget["s"] = UITextSpan::NewStrikethrough;
		registeredWidget["strike"] = [] { return UITextSpan::NewWithTag( "strike" ); };
		registeredWidget["del"] = UITextSpan::NewStrikethrough;
		registeredWidget["font"] = UITextSpan::NewFont;
		registeredWidget["code"] = UITextSpan::NewCode;
		registeredWidget["abbr"] = [] { return UITextSpan::NewWithTag( "abbr" ); };
		registeredWidget["tt"] = [] { return UITextSpan::NewWithTag( "tt" ); };
		registeredWidget["mark"] = UITextSpan::NewMark;
		registeredWidget["div"] = UIRichText::NewDiv;
		registeredWidget["p"] = UIRichText::NewParagraph;
		registeredWidget["blockquote"] = UIRichText::NewBlockquote;
		registeredWidget["h1"] = UIRichText::NewH1;
		registeredWidget["h2"] = UIRichText::NewH2;
		registeredWidget["h3"] = UIRichText::NewH3;
		registeredWidget["h4"] = UIRichText::NewH4;
		registeredWidget["h5"] = UIRichText::NewH5;
		registeredWidget["h6"] = UIRichText::NewH6;
		registeredWidget["br"] = UIRichText::NewBr;
		registeredWidget["hr"] = UIRichText::NewHr;
		registeredWidget["ul"] = [] { return UIRichText::NewWithTag( "ul" ); };
		registeredWidget["ol"] = [] { return UIRichText::NewWithTag( "ol" ); };
		registeredWidget["dl"] = [] { return UIRichText::NewWithTag( "dl" ); };
		registeredWidget["dt"] = [] { return UIRichText::NewWithTag( "dt" ); };
		registeredWidget["dd"] = [] { return UIRichText::NewWithTag( "dd" ); };
		registeredWidget["li"] = UIHTMLListItem::New;
		registeredWidget["details"] = UIHTMLDetails::New;
		registeredWidget["summary"] = UIHTMLSummary::New;
		registeredWidget["pre"] = UIRichText::NewPre;
		registeredWidget["picture"] = [] { return UITextSpan::NewWithTag( "picture" ); };
		registeredWidget["img"] = UIHTMLImage::New;
		registeredWidget["svg"] = [] {
			auto svg = UISvg::New();
			svg->setFlags( UI_HTML_ELEMENT );
			return svg;
		};
		registeredWidget["input"] = [] { return UIHTMLInput::New(); };
		registeredWidget["header"] = [] { return UIRichText::NewWithTag( "header" ); };
		registeredWidget["article"] = [] { return UIRichText::NewWithTag( "article" ); };
		registeredWidget["figure"] = [] { return UIRichText::NewWithTag( "figure" ); };
		registeredWidget["figcaption"] = [] { return UIRichText::NewWithTag( "figcaption" ); };
		registeredWidget["footer"] = [] { return UIRichText::NewWithTag( "footer" ); };
		registeredWidget["main"] = [] { return UIRichText::NewWithTag( "main" ); };
		registeredWidget["section"] = [] { return UIRichText::NewWithTag( "section" ); };
		registeredWidget["nav"] = [] { return UIRichText::NewWithTag( "nav" ); };
		registeredWidget["center"] = [] {
			auto center = UIRichText::NewWithTag( "center" );
			center->setTextAlign( TEXT_ALIGN_CENTER );
			center->on( Event::OnChildCountChanged, []( const Event* event ) {
				if ( !event->asChildCountChangedEvent()->removed() &&
					 event->asChildCountChangedEvent()->child()->isType( UI_TYPE_HTML_WIDGET ) ) {
					event->asChildCountChangedEvent()
						->child()
						->asType<UIHTMLWidget>()
						->applyProperty( StyleSheetProperty( "text-align", "center" ) );
				}
			} );
			return center;
		};
		registeredWidget["aside"] = [] { return UIRichText::NewWithTag( "aside" ); };
		registeredWidget["html"] = UIRichText::NewHtml;
		registeredWidget["head"] = UIHTMLHead::New;
		registeredWidget["body"] = UIRichText::NewBody;
		registeredWidget["form"] = [] { return UIHTMLForm::New(); };
		registeredWidget["table"] = UIHTMLTable::New;
		registeredWidget["tr"] = UIHTMLTableRow::New;
		registeredWidget["thead"] = UIHTMLTableHead::New;
		registeredWidget["tbody"] = UIHTMLTableBody::New;
		registeredWidget["tfoot"] = UIHTMLTableFooter::New;
		registeredWidget["th"] = [] { return UIHTMLTableCell::New( "th" ); };
		registeredWidget["td"] = [] { return UIHTMLTableCell::New( "td" ); };
		registeredWidget["input"] = UIHTMLInput::New;
		registeredWidget["textarea"] = UIHTMLTextArea::New;
		registeredWidget["button"] = [] { return UIRichText::NewWithTag( "button" ); };
		registeredWidget["webview"] = UIWebView::New;

		sBaseListCreated = true;
	}
}

UIWidget* UIWidgetCreator::createFromName( const std::string& widgetName ) {
	createBaseWidgetList();

	if ( widgetName.empty() )
		return nullptr;

	std::string lwidgetName( String::toLower( widgetName ) );

	if ( registeredWidget.find( lwidgetName ) != registeredWidget.end() ) {
		return registeredWidget[lwidgetName]();
	}

	if ( widgetCallback.find( lwidgetName ) != widgetCallback.end() ) {
		return widgetCallback[lwidgetName]( lwidgetName );
	}

	eePRINTL( "UIWidgetCreator::createFromName: \"%s\" not found", widgetName.c_str() );

	return nullptr;
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

static void loadHTMLDefaults( CSS::StyleSheet& styleSheet, Uint32 marker, std::string_view css ) {
	if ( styleSheet.markerExists( marker ) )
		return;
	CSS::StyleSheetParser parser;
	if ( parser.loadFromString( css ) ) {
		CSS::StyleSheet& baseDefaults = parser.getStyleSheet();
		baseDefaults.setSelectorSpecificity( -1 );
		baseDefaults.setMarker( marker );
		styleSheet.combineStyleSheet( baseDefaults );
	}
}

void UIWidgetCreator::loadHTMLBasicDefaults( CSS::StyleSheet& styleSheet, Uint32 marker ) {
	loadHTMLDefaults( styleSheet, marker, getHTMLBasicDefaultsCSS() );
}

void UIWidgetCreator::loadHTMLBaseDefaults( CSS::StyleSheet& styleSheet, Uint32 marker ) {
	loadHTMLBasicDefaults( styleSheet, marker );
	loadHTMLDefaults( styleSheet, String::hash( "html_document_defaults" ),
					  getHTMLDocumentDefaultsCSS() );
}

}} // namespace EE::UI
