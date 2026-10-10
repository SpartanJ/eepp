#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/accessibility/accessibilitywidgetresolver.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenuradiobutton.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

#include <algorithm>

namespace EE { namespace UI {

namespace {

bool isRoot( const UIWidget* widget ) {
	return widget->getUISceneNode() && !widget->getUISceneNode()->getParent() &&
		   widget == widget->getUISceneNode()->getRoot();
}

bool isComboBoxExpanded( const UIComboBox* widget ) {
	const auto* popup = widget->getListBox();
	// Fade-out keeps the popup visible, but disabling it closes the interactive control.
	return popup->isVisible() && popup->isEnabled();
}

AccessibilityState baseState( const UIWidget* widget ) {
	AccessibilityState state = AccessibilityState::None;
	if ( widget->isEnabled() )
		state |= AccessibilityState::Enabled;
	if ( widget->isTabFocusable() )
		state |= AccessibilityState::Focusable;
	auto* dispatcher = widget->getEventDispatcher();
	auto* focused = dispatcher ? dispatcher->getFocusNode() : nullptr;
	// Only the projected focus owner is focused; its ancestors merely contain the focus.
	if ( focused && ( widget == focused || widget->isParentOf( focused ) ) ) {
		const auto* scene = widget->getUISceneNode();
		const auto* manager = scene ? scene->getAccessibilityManager() : nullptr;
		const auto* managerScene = manager ? manager->getSceneNode() : scene;
		if ( AccessibilityWidgetResolver::getFocusOwner(
				 focused, managerScene ? managerScene->getRoot() : nullptr ) == widget )
			state |= AccessibilityState::Focused;
	}
	if ( widget->isVisible() )
		state |= AccessibilityState::Visible;
	if ( widget->hasVisibility() )
		state |= AccessibilityState::Showing;
	auto role = AccessibilityWidgetResolver::getRole( widget );
	if ( ( role == AccessibilityRole::Application || role == AccessibilityRole::Window ) &&
		 widget->getUISceneNode() && widget->getUISceneNode()->getWindow()->hasInputFocus() )
		state |= AccessibilityState::Active;
	return state;
}

AccessibilityActions baseActions( const UIWidget* widget ) {
	return widget->isTabFocusable() ? accessibilityActionMask( AccessibilityAction::Focus ) : 0;
}

/** The nearest scroll view whose scrolled content contains the widget. */
UIScrollView* scrollViewFor( const UIWidget* widget ) {
	for ( auto* parent = widget->getParent(); parent; parent = parent->getParent() ) {
		if ( parent->isType( UI_TYPE_SCROLLVIEW ) ) {
			auto* view = parent->asType<UIScrollView>();
			if ( view->getScrollView() && view->getScrollView()->isParentOf( widget ) )
				return view;
		}
	}
	return nullptr;
}

/** Start offsets of every line of one document version, plus the total length. Offset and
 * position conversions become an index lookup or a binary search instead of a walk over every
 * line (each line's size takes the document mutex). Rebuilt once per text revision. */
class LineIndex {
  public:
	const std::vector<Int64>& starts( const Doc::TextDocument& document ) {
		if ( document.getUUID() != mDocument || document.getModificationId() != mModification ||
			 mStarts.empty() ) {
			mDocument = document.getUUID();
			mModification = document.getModificationId();
			const size_t lines = document.linesCount();
			mStarts.resize( lines + 1 );
			mStarts[0] = 0;
			for ( size_t line = 0; line < lines; ++line )
				mStarts[line + 1] =
					mStarts[line] + static_cast<Int64>( document.line( line ).size() );
		}
		return mStarts;
	}

	bool matches( const Doc::TextDocument& document ) const {
		return document.getUUID() == mDocument && document.getModificationId() == mModification &&
			   !mStarts.empty();
	}

  private:
	System::UUID mDocument{ 0, 0 };
	Uint64 mModification{ 0 };
	std::vector<Int64> mStarts;
};

/** UI thread only. Two entries let a client alternate between an editor and a field. */
const std::vector<Int64>& lineStarts( const Doc::TextDocument& document ) {
	static LineIndex indexes[2];
	static size_t next = 0;
	for ( auto& index : indexes ) {
		if ( index.matches( document ) )
			return index.starts( document );
	}
	auto& index = indexes[next];
	next = ( next + 1 ) % 2;
	return index.starts( document );
}

Int32 documentOffset( const Doc::TextDocument& document, const Doc::TextPosition& position ) {
	if ( !position.isValid() )
		return 0;
	const auto& starts = lineStarts( document );
	const size_t line =
		std::min( static_cast<size_t>( std::max<Int64>( 0, position.line() ) ), starts.size() - 1 );
	return static_cast<Int32>( std::max<Int64>( 0, starts[line] + position.column() ) );
}

Doc::TextPosition documentPosition( const Doc::TextDocument& document, Int32 offset ) {
	const auto& starts = lineStarts( document );
	if ( starts.size() < 2 )
		return { 0, 0 };
	const Int64 target = std::max<Int32>( 0, offset );
	// The last line whose start is at or before the offset; past the end clamps to the last line.
	const size_t lines = starts.size() - 1;
	size_t line = static_cast<size_t>( std::upper_bound( starts.begin(), starts.end(), target ) -
									   starts.begin() ) -
				  1;
	line = std::min( line, lines - 1 );
	const Int64 length = starts[line + 1] - starts[line];
	return { static_cast<Int64>( line ), std::min( target - starts[line], length ) };
}

AccessibilityTextInfo getDocumentText( const Doc::TextDocument& document ) {
	// A document loading on a worker thread is still being filled in.
	if ( document.isLoading() )
		return { 0, 0, 0, true };
	auto selection = document.getSelection( true );
	return { documentOffset( document, document.getSelection().end() ),
			 documentOffset( document, selection.start() ),
			 documentOffset( document, selection.end() ), true };
}

bool parseSelection( const String& value, Int32& start, Int32& end ) {
	auto parts = String::split( value, ':' );
	return parts.size() == 2 && String::fromString( start, parts[0].toUtf8() ) &&
		   String::fromString( end, parts[1].toUtf8() );
}

} // namespace

AccessibilityRole AccessibilityWidgetResolver::getRole( const UIWidget* widget ) {
	AccessibilityRole role = AccessibilityRole::None;
	if ( isRoot( widget ) )
		role = AccessibilityRole::Application;
	else if ( widget->isType( UI_TYPE_MENUCHECKBOX ) )
		role = AccessibilityRole::CheckMenuItem;
	else if ( widget->isType( UI_TYPE_MENURADIOBUTTON ) )
		role = AccessibilityRole::RadioMenuItem;
	else if ( widget->isType( UI_TYPE_MENUITEM ) )
		role = AccessibilityRole::MenuItem;
	else if ( widget->isType( UI_TYPE_MENUBAR ) )
		role = AccessibilityRole::MenuBar;
	else if ( widget->isType( UI_TYPE_MENU ) )
		role = AccessibilityRole::Menu;
	else if ( widget->isType( UI_TYPE_LISTVIEW ) )
		role = AccessibilityRole::List;
	else if ( widget->isType( UI_TYPE_TREEVIEW ) )
		role = AccessibilityRole::Tree;
	else if ( widget->isType( UI_TYPE_TABLEVIEW ) )
		role = AccessibilityRole::Table;
	else if ( widget->isType( UI_TYPE_CHECKBOX ) )
		role = AccessibilityRole::CheckBox;
	else if ( widget->isType( UI_TYPE_RADIOBUTTON ) )
		role = AccessibilityRole::RadioButton;
	else if ( widget->isType( UI_TYPE_TAB ) )
		role = AccessibilityRole::Tab;
	else if ( widget->isType( UI_TYPE_TABWIDGET ) )
		role = AccessibilityRole::TabList;
	else if ( widget->isType( UI_TYPE_COMBOBOX ) )
		role = AccessibilityRole::ComboBox;
	else if ( widget->isType( UI_TYPE_CODEEDITOR ) )
		role = AccessibilityRole::TextBox;
	else if ( widget->isType( UI_TYPE_TEXTINPUT ) )
		role = AccessibilityRole::TextBox;
	else if ( widget->isType( UI_TYPE_SLIDER ) )
		role = AccessibilityRole::Slider;
	else if ( widget->isType( UI_TYPE_SPINBOX ) )
		role = AccessibilityRole::SpinButton;
	else if ( widget->isType( UI_TYPE_PROGRESSBAR ) )
		role = AccessibilityRole::ProgressBar;
	else if ( widget->isType( UI_TYPE_WINDOW ) )
		role = AccessibilityRole::Window;
	else if ( widget->isType( UI_TYPE_PUSHBUTTON ) )
		role = AccessibilityRole::Button;
	else if ( widget->isType( UI_TYPE_TEXTVIEW ) )
		role = AccessibilityRole::Label;
	return widget->resolveAccessibilityRole( role );
}

bool AccessibilityWidgetResolver::isLeafRole( AccessibilityRole role ) {
	switch ( role ) {
		case AccessibilityRole::Button:
		case AccessibilityRole::CheckBox:
		case AccessibilityRole::RadioButton:
		case AccessibilityRole::Label:
		case AccessibilityRole::Text:
		case AccessibilityRole::Image:
		case AccessibilityRole::Slider:
		case AccessibilityRole::SpinButton:
		case AccessibilityRole::ProgressBar:
		case AccessibilityRole::Tab:
		case AccessibilityRole::MenuItem:
		case AccessibilityRole::CheckMenuItem:
		case AccessibilityRole::RadioMenuItem:
			return true;
		default:
			return false;
	}
}

UIWidget* AccessibilityWidgetResolver::getLeafOwner( const Node* node ) {
	UIWidget* owner = nullptr;
	for ( auto* parent = node->getParent(); parent; parent = parent->getParent() ) {
		if ( parent->isWidget() && isLeafRole( getRole( parent->asType<UIWidget>() ) ) )
			owner = parent->asType<UIWidget>();
	}
	return owner;
}

bool AccessibilityWidgetResolver::isHiddenFromAccessibility( const UIWidget* widget ) {
	for ( const Node* node = widget; node; node = node->getParent() ) {
		if ( node->isWidget() && static_cast<const UIWidget*>( node )->isAccessibilityHidden() )
			return true;
	}
	return false;
}

String AccessibilityWidgetResolver::getOwnName( const UIWidget* widget ) {
	String name;
	if ( isRoot( widget ) && widget->getUISceneNode()->getWindow() )
		name = String::fromUtf8( widget->getUISceneNode()->getWindow()->getTitle() );
	else if ( widget->isType( UI_TYPE_PUSHBUTTON ) )
		name = static_cast<const UIPushButton*>( widget )->getText();
	else if ( widget->isType( UI_TYPE_TEXTVIEW ) && !widget->isType( UI_TYPE_TEXTINPUT ) &&
			  !widget->isType( UI_TYPE_TEXTEDIT ) )
		name = static_cast<const UITextView*>( widget )->getText();
	else if ( widget->isType( UI_TYPE_CODEEDITOR ) &&
			  static_cast<const UICodeEditor*>( widget )->getDocument().hasFilepath() )
		name = String::fromUtf8(
			static_cast<const UICodeEditor*>( widget )->getDocument().getFilename() );
	name = widget->resolveAccessibilityName( name );
	// Icon-only buttons usually carry their meaning in the tooltip.
	return name.empty() ? widget->getTooltipText() : name;
}

namespace {

/** Resolves an aria-labelledby / aria-describedby id from the widget's scene root. */
const UIWidget* findRelated( const UIWidget* widget, const std::string& id ) {
	if ( id.empty() || !widget->getUISceneNode() || !widget->getUISceneNode()->getRoot() )
		return nullptr;
	Node* node = widget->getUISceneNode()->getRoot()->find( id );
	return node && node != widget && node->isWidget() ? node->asType<UIWidget>() : nullptr;
}

} // namespace

const char* AccessibilityWidgetResolver::getRoleName( AccessibilityRole role ) {
	switch ( role ) {
		case AccessibilityRole::None:
			return "None";
		case AccessibilityRole::Application:
			return "Application";
		case AccessibilityRole::Window:
			return "Window";
		case AccessibilityRole::Dialog:
			return "Dialog";
		case AccessibilityRole::Group:
			return "Group";
		case AccessibilityRole::Button:
			return "Button";
		case AccessibilityRole::CheckBox:
			return "CheckBox";
		case AccessibilityRole::RadioButton:
			return "RadioButton";
		case AccessibilityRole::Label:
			return "Label";
		case AccessibilityRole::Text:
			return "Text";
		case AccessibilityRole::TextBox:
			return "TextBox";
		case AccessibilityRole::Image:
			return "Image";
		case AccessibilityRole::ComboBox:
			return "ComboBox";
		case AccessibilityRole::Slider:
			return "Slider";
		case AccessibilityRole::SpinButton:
			return "SpinButton";
		case AccessibilityRole::ProgressBar:
			return "ProgressBar";
		case AccessibilityRole::TabList:
			return "TabList";
		case AccessibilityRole::Tab:
			return "Tab";
		case AccessibilityRole::TabPanel:
			return "TabPanel";
		case AccessibilityRole::MenuBar:
			return "MenuBar";
		case AccessibilityRole::Menu:
			return "Menu";
		case AccessibilityRole::MenuItem:
			return "MenuItem";
		case AccessibilityRole::CheckMenuItem:
			return "CheckMenuItem";
		case AccessibilityRole::RadioMenuItem:
			return "RadioMenuItem";
		case AccessibilityRole::List:
			return "List";
		case AccessibilityRole::ListItem:
			return "ListItem";
		case AccessibilityRole::Table:
			return "Table";
		case AccessibilityRole::Row:
			return "Row";
		case AccessibilityRole::Cell:
			return "Cell";
		case AccessibilityRole::Tree:
			return "Tree";
		case AccessibilityRole::TreeItem:
			return "TreeItem";
	}
	return "Unknown";
}

String AccessibilityWidgetResolver::getName( const UIWidget* widget ) {
	if ( const auto* label = findRelated( widget, widget->getAccessibilityLabelledBy() ) ) {
		String name = getOwnName( label );
		if ( !name.empty() )
			return name;
	}
	return getOwnName( widget );
}

String AccessibilityWidgetResolver::getDescription( const UIWidget* widget ) {
	if ( const auto* description = findRelated( widget, widget->getAccessibilityDescribedBy() ) ) {
		String text = getOwnName( description );
		if ( !text.empty() )
			return text;
	}
	String description = widget->resolveAccessibilityDescription();
	if ( !description.empty() )
		return description;
	// A tooltip that did not become the name still explains the control.
	const String tooltip = widget->getTooltipText();
	return tooltip.empty() || tooltip == getOwnName( widget ) ? String() : tooltip;
}

AccessibilityShortcut AccessibilityWidgetResolver::getShortcut( const UIWidget* widget ) {
	AccessibilityShortcut result;
	if ( !widget->isType( UI_TYPE_MENUITEM ) )
		return result;
	const auto* item = static_cast<const UIMenuItem*>( widget );
	const auto* view = item->getShortcutView();
	if ( !view || view->getText().empty() )
		return result;
	result.text = view->getText().toUtf8();
	const auto& shortcut = item->getShortcut();
	if ( shortcut.key == KEY_UNKNOWN || !widget->getInput() )
		return result;
	result.key = widget->getInput()->getKeyName( shortcut.key );
	if ( shortcut.mod & KEYMOD_CTRL )
		result.modifiers |= AccessibilityShortcut::Control;
	if ( shortcut.mod & KEYMOD_SHIFT )
		result.modifiers |= AccessibilityShortcut::Shift;
	if ( shortcut.mod & KEYMOD_ALT )
		result.modifiers |= AccessibilityShortcut::Alt;
	if ( shortcut.mod & KEYMOD_META )
		result.modifiers |= AccessibilityShortcut::Meta;
	return result;
}

String AccessibilityWidgetResolver::getValue( const UIWidget* widget ) {
	if ( widget->isType( UI_TYPE_COMBOBOX ) )
		return static_cast<const UIComboBox*>( widget )->getDropDownList()->getText();
	if ( widget->isType( UI_TYPE_CODEEDITOR ) )
		return static_cast<const UICodeEditor*>( widget )->getDocument().getText();
	if ( widget->isType( UI_TYPE_TEXTINPUT ) ) {
		auto input = static_cast<const UITextInput*>( widget );
		return input->getMode() == UITextInput::TextInputMode::Password ? String()
																		: input->getText();
	}
	if ( widget->isType( UI_TYPE_SLIDER ) )
		return String( String::toString( static_cast<const UISlider*>( widget )->getValue() ) );
	if ( widget->isType( UI_TYPE_SPINBOX ) )
		return String( String::toString( static_cast<const UISpinBox*>( widget )->getValue() ) );
	if ( widget->isType( UI_TYPE_PROGRESSBAR ) )
		return String(
			String::toString( static_cast<const UIProgressBar*>( widget )->getProgress() ) );
	return {};
}

AccessibilityRangeInfo AccessibilityWidgetResolver::getRange( const UIWidget* widget ) {
	if ( widget->isType( UI_TYPE_SLIDER ) ) {
		auto slider = static_cast<const UISlider*>( widget );
		return { slider->getMinValue(), slider->getMaxValue(), slider->getClickStep(),
				 slider->getPageStep(), true };
	}
	if ( widget->isType( UI_TYPE_SPINBOX ) ) {
		auto spinBox = static_cast<const UISpinBox*>( widget );
		return { spinBox->getMinValue(), spinBox->getMaxValue(), spinBox->getClickStep(),
				 spinBox->getClickStep(), true };
	}
	if ( widget->isType( UI_TYPE_PROGRESSBAR ) )
		return { 0., static_cast<const UIProgressBar*>( widget )->getTotalSteps(), 0., 0., true };
	return {};
}

AccessibilityTextInfo AccessibilityWidgetResolver::getText( const UIWidget* widget ) {
	return getText( widget, true );
}

const Doc::TextDocument* AccessibilityWidgetResolver::getTextDocument( const UIWidget* widget ) {
	const Doc::TextDocument* document = nullptr;
	if ( widget->isType( UI_TYPE_CODEEDITOR ) ) {
		document = &static_cast<const UICodeEditor*>( widget )->getDocument();
	} else if ( widget->isType( UI_TYPE_TEXTINPUT ) ) {
		const auto* input = static_cast<const UITextInput*>( widget );
		if ( input->getMode() != UITextInput::TextInputMode::Password )
			document = &input->getDocument();
	}
	// A document loading on a worker thread is still being filled in.
	return document && !document->isLoading() ? document : nullptr;
}

Int32 AccessibilityWidgetResolver::getTextLength( const UIWidget* widget ) {
	const auto* document = getTextDocument( widget );
	if ( !document )
		return 0;
	// The exposed text stops before the last line's newline, like TextDocument::getText().
	return static_cast<Int32>( std::max<Int64>( 0, lineStarts( *document ).back() - 1 ) );
}

String AccessibilityWidgetResolver::getTextRange( const UIWidget* widget, Int32 start, Int32 end ) {
	const auto* document = getTextDocument( widget );
	if ( !document || end <= start )
		return {};
	return document->getText(
		{ documentPosition( *document, start ), documentPosition( *document, end ) } );
}

bool AccessibilityWidgetResolver::getTextLineBounds( const UIWidget* widget, Int32 offset,
													 Int32& start, Int32& end ) {
	const auto* document = getTextDocument( widget );
	if ( !document || document->linesCount() == 0 )
		return false;
	const auto position = documentPosition( *document, offset );
	start = static_cast<Int32>( std::max<Int64>( 0, offset - position.column() ) );
	const Int64 lineLength = static_cast<Int64>( document->line( position.line() ).size() );
	const bool lastLine = position.line() + 1 == static_cast<Int64>( document->linesCount() );
	// The last line's newline is not part of the exposed text.
	end =
		start + static_cast<Int32>( lastLine ? std::max<Int64>( 0, lineLength - 1 ) : lineLength );
	return true;
}

AccessibilityTextRevision AccessibilityWidgetResolver::getTextRevision( const UIWidget* widget ) {
	const auto* document = getTextDocument( widget );
	return document
			   ? AccessibilityTextRevision{ document->getUUID().high(), document->getUUID().low(),
											document->getModificationId() }
			   : AccessibilityTextRevision{};
}

Int32 AccessibilityWidgetResolver::getTextOffset( const Doc::TextDocument& document,
												  const Doc::TextPosition& position ) {
	return documentOffset( document, position );
}

AccessibilityTextInfo AccessibilityWidgetResolver::getText( const UIWidget* widget,
															bool includeOffsets ) {
	if ( widget->isType( UI_TYPE_CODEEDITOR ) ) {
		return includeOffsets
				   ? getDocumentText( static_cast<const UICodeEditor*>( widget )->getDocument() )
				   : AccessibilityTextInfo{ 0, 0, 0, true };
	}
	if ( widget->isType( UI_TYPE_TEXTINPUT ) ) {
		const auto* input = static_cast<const UITextInput*>( widget );
		return input->getMode() == UITextInput::TextInputMode::Password ? AccessibilityTextInfo{}
			   : includeOffsets ? getDocumentText( input->getDocument() )
								: AccessibilityTextInfo{ 0, 0, 0, true };
	}
	return {};
}

AccessibilityState AccessibilityWidgetResolver::getState( const UIWidget* widget ) {
	auto state = baseState( widget );
	if ( widget->isType( UI_TYPE_MENUCHECKBOX ) &&
		 static_cast<const UIMenuCheckBox*>( widget )->isActive() )
		state |= AccessibilityState::Checked;
	if ( widget->isType( UI_TYPE_MENURADIOBUTTON ) &&
		 static_cast<const UIMenuRadioButton*>( widget )->isActive() ) {
		state |= AccessibilityState::Selected;
		state |= AccessibilityState::Checked;
	}
	if ( widget->isType( UI_TYPE_CHECKBOX ) &&
		 static_cast<const UICheckBox*>( widget )->isChecked() )
		state |= AccessibilityState::Checked;
	if ( widget->isType( UI_TYPE_RADIOBUTTON ) &&
		 static_cast<const UIRadioButton*>( widget )->isActive() ) {
		state |= AccessibilityState::Checked;
		state |= AccessibilityState::Selected;
	}
	if ( widget->isType( UI_TYPE_SELECTBUTTON ) &&
		 static_cast<const UISelectButton*>( widget )->isSelected() )
		state |= AccessibilityState::Selected;
	if ( widget->isType( UI_TYPE_COMBOBOX ) &&
		 isComboBoxExpanded( static_cast<const UIComboBox*>( widget ) ) )
		state |= AccessibilityState::Expanded;
	if ( widget->isType( UI_TYPE_CODEEDITOR ) ) {
		state |= AccessibilityState::MultiLine;
		state |= static_cast<const UICodeEditor*>( widget )->isLocked()
					 ? AccessibilityState::ReadOnly
					 : AccessibilityState::Editable;
	} else if ( widget->isType( UI_TYPE_TEXTINPUT ) ) {
		state |= static_cast<const UITextInput*>( widget )->isEditingAllowed()
					 ? AccessibilityState::Editable
					 : AccessibilityState::ReadOnly;
		if ( static_cast<const UITextInput*>( widget )->getMode() ==
			 UITextInput::TextInputMode::Password )
			state |= AccessibilityState::Protected;
	}
	return state;
}

AccessibilityActions AccessibilityWidgetResolver::getActions( const UIWidget* widget ) {
	// Anything inside a scroll view, labels included, can be brought into view.
	const AccessibilityActions scroll =
		scrollViewFor( widget ) ? accessibilityActionMask( AccessibilityAction::ScrollTo ) : 0;
	if ( isRoot( widget ) ||
		 ( widget->isType( UI_TYPE_TEXTVIEW ) && !widget->isType( UI_TYPE_CHECKBOX ) &&
		   !widget->isType( UI_TYPE_RADIOBUTTON ) && !widget->isType( UI_TYPE_TEXTINPUT ) ) )
		return scroll;
	auto actions = baseActions( widget ) | scroll;
	if ( widget->isType( UI_TYPE_MENUCHECKBOX ) ) {
		actions |= accessibilityActionMask( AccessibilityAction::Toggle );
	} else if ( widget->isType( UI_TYPE_MENURADIOBUTTON ) ) {
		actions |= accessibilityActionMask( AccessibilityAction::Select );
	} else if ( widget->isType( UI_TYPE_TAB ) || widget->isType( UI_TYPE_SELECTBUTTON ) ) {
		actions |= accessibilityActionMask( AccessibilityAction::Select );
	} else if ( widget->isType( UI_TYPE_PUSHBUTTON ) ) {
		actions |= accessibilityActionMask( AccessibilityAction::Press );
	} else if ( widget->isType( UI_TYPE_MENUITEM ) ) {
		actions |= accessibilityActionMask( AccessibilityAction::Press );
	}
	if ( widget->isType( UI_TYPE_CHECKBOX ) )
		actions |= accessibilityActionMask( AccessibilityAction::Toggle );
	if ( widget->isType( UI_TYPE_RADIOBUTTON ) )
		actions |= accessibilityActionMask( AccessibilityAction::Select );
	if ( widget->isType( UI_TYPE_CODEEDITOR ) &&
		 !static_cast<const UICodeEditor*>( widget )->isLocked() )
		actions |= accessibilityActionMask( AccessibilityAction::SetText );
	else if ( widget->isType( UI_TYPE_TEXTINPUT ) &&
			  static_cast<const UITextInput*>( widget )->isEditingAllowed() )
		actions |= accessibilityActionMask( AccessibilityAction::SetText );
	if ( ( widget->isType( UI_TYPE_TEXTINPUT ) &&
		   static_cast<const UITextInput*>( widget )->isEditingAllowed() &&
		   static_cast<const UITextInput*>( widget )->getMode() !=
			   UITextInput::TextInputMode::Password ) ||
		 ( widget->isType( UI_TYPE_CODEEDITOR ) &&
		   !static_cast<const UICodeEditor*>( widget )->isLocked() ) )
		actions |= accessibilityActionMask( AccessibilityAction::SetTextSelection );
	if ( widget->isType( UI_TYPE_COMBOBOX ) ) {
		actions |=
			accessibilityActionMask( isComboBoxExpanded( static_cast<const UIComboBox*>( widget ) )
										 ? AccessibilityAction::Collapse
										 : AccessibilityAction::Expand );
	}
	if ( widget->isType( UI_TYPE_SLIDER ) || widget->isType( UI_TYPE_SPINBOX ) )
		actions |= accessibilityActionMask( AccessibilityAction::Increment ) |
				   accessibilityActionMask( AccessibilityAction::Decrement ) |
				   accessibilityActionMask( AccessibilityAction::SetValue );
	return actions;
}

UIWidget* AccessibilityWidgetResolver::getOwningModelView( const Node* node ) {
	for ( auto* parent = node->getParent(); parent; parent = parent->getParent() ) {
		if ( !parent->isWidget() || !parent->isType( UI_TYPE_ABSTRACTTABLEVIEW ) )
			continue;
		auto* view = parent->asType<UIWidget>();
		// The active cell editor is a real widget embedded below its virtual row or cell.
		const auto* editor = static_cast<const Abstract::UIAbstractView*>( view )->getEditWidget();
		if ( editor && ( editor == node || editor->isParentOf( node ) ) )
			return nullptr;
		return view;
	}
	return nullptr;
}

UIWidget* AccessibilityWidgetResolver::getFocusOwner( Node* focused, const UIWidget* sceneRoot ) {
	UIWidget* element = nullptr;
	// Semantic boundaries hide implementation children (for example a spin box's input).
	// Project focus through exactly the same boundaries as getChildren() and hitTest().
	for ( auto* node = focused; node; node = node->getParent() ) {
		if ( node->isDestroying() )
			return nullptr;
		if ( !node->isWidget() )
			continue;
		auto* widget = node->asType<UIWidget>();
		if ( widget->isAccessibilityHidden() )
			element = nullptr;
		else if ( widget->isType( UI_TYPE_ABSTRACTTABLEVIEW ) ) {
			// Recycled cell widgets are represented by the view; its cell editor is not.
			if ( !element || getOwningModelView( element ) == widget )
				element = widget;
		} else if ( !element && widget != sceneRoot && widget->isAccessibilityElement() )
			element = widget;
		else if ( element && widget != element && isLeafRole( getRole( widget ) ) )
			// A leaf control does not expose its children; it owns their focus.
			element = widget;
	}
	return element;
}

UIWidget* AccessibilityWidgetResolver::getEventTarget( UIWidget* widget,
													   AccessibilityEvent event ) {
	const Node* top = widget;
	for ( auto* parent = widget->getParent(); parent; parent = parent->getParent() ) {
		if ( parent->isDestroying() )
			return nullptr;
		if ( parent->isWidget() && parent->asType<UIWidget>()->isAccessibilityHidden() )
			return nullptr;
		top = parent;
	}
	// Detached widgets (for example an editor configured before it is parented) are not part of
	// any accessible tree yet; notifying them would only register orphan identities.
	if ( !top->isSceneNode() || widget->isAccessibilityHidden() )
		return nullptr;
	if ( auto* view = getOwningModelView( widget ) )
		return event == AccessibilityEvent::FocusChanged ? view : nullptr;
	if ( auto* leaf = getLeafOwner( widget ) ) {
		// A leaf control's text view changing its text is the control's name changing. Structural
		// events stay invisible: the control has no exposed children.
		return event == AccessibilityEvent::ChildrenChanged ? nullptr : leaf;
	}
	if ( event == AccessibilityEvent::StateChanged && widget->isType( UI_TYPE_DROPDOWN ) ) {
		auto* parent = widget->getParent();
		if ( parent && parent->isType( UI_TYPE_COMBOBOX ) )
			return parent->asType<UIWidget>();
	}
	return widget;
}

namespace {

void auditWidget( const UIWidget* widget, std::vector<AccessibilityIssue>& issues ) {
	// Recycled model-view cells are exposed through the view's virtual rows, not as widgets.
	if ( widget->isAccessibilityHidden() || !widget->isVisible() ||
		 AccessibilityWidgetResolver::getOwningModelView( widget ) )
		return;
	const auto role = AccessibilityWidgetResolver::getRole( widget );
	if ( role != AccessibilityRole::None && role != AccessibilityRole::Application &&
		 role != AccessibilityRole::Label && widget->isTabFocusable() &&
		 AccessibilityWidgetResolver::getName( widget ).empty() &&
		 AccessibilityWidgetResolver::getValue( widget ).empty() )
		issues.push_back( { widget, "Focusable control has no accessible name (set aria-label, "
									"aria-labelledby or a tooltip)." } );
	if ( !widget->getAccessibilityLabelledBy().empty() &&
		 !findRelated( widget, widget->getAccessibilityLabelledBy() ) )
		issues.push_back( { widget, "aria-labelledby \"" +
										String( widget->getAccessibilityLabelledBy() ) +
										"\" does not match any widget." } );
	if ( !widget->getAccessibilityDescribedBy().empty() &&
		 !findRelated( widget, widget->getAccessibilityDescribedBy() ) )
		issues.push_back( { widget, "aria-describedby \"" +
										String( widget->getAccessibilityDescribedBy() ) +
										"\" does not match any widget." } );
	// A leaf control's children are implementation details that no client can reach.
	if ( AccessibilityWidgetResolver::isLeafRole( role ) )
		return;
	for ( const Node* child = widget->getFirstChild(); child; child = child->getNextNode() ) {
		if ( child->isWidget() )
			auditWidget( static_cast<const UIWidget*>( child ), issues );
	}
}

} // namespace

std::vector<AccessibilityIssue> AccessibilityWidgetResolver::audit( const UIWidget* root ) {
	std::vector<AccessibilityIssue> issues;
	if ( root )
		auditWidget( root, issues );
	return issues;
}

bool AccessibilityWidgetResolver::performAction( UIWidget* widget,
												 const AccessibilityActionRequest& request ) {
	if ( !( getActions( widget ) & accessibilityActionMask( request.action ) ) )
		return false;
	if ( request.action == AccessibilityAction::Focus ) {
		widget->setFocus( NodeFocusReason::Unknown );
		return true;
	}
	if ( request.action == AccessibilityAction::ScrollTo ) {
		// Bring the widget into view through every enclosing scroll view, innermost first.
		bool scrolled = false;
		for ( auto* view = scrollViewFor( widget ); view; view = scrollViewFor( view ) )
			scrolled |= view->scrollIntoView( widget );
		return scrolled;
	}
	if ( request.action == AccessibilityAction::Toggle && widget->isType( UI_TYPE_MENUCHECKBOX ) ) {
		static_cast<UIMenuCheckBox*>( widget )->activate();
		return true;
	}
	if ( request.action == AccessibilityAction::Select &&
		 widget->isType( UI_TYPE_MENURADIOBUTTON ) ) {
		static_cast<UIMenuRadioButton*>( widget )->activate();
		return true;
	}
	if ( request.action == AccessibilityAction::Press && widget->isType( UI_TYPE_MENUITEM ) ) {
		static_cast<UIMenuItem*>( widget )->activate();
		return true;
	}
	if ( request.action == AccessibilityAction::Select && widget->isType( UI_TYPE_TAB ) ) {
		auto tab = static_cast<UITab*>( widget );
		if ( auto tabWidget = tab->getTabWidget() ) {
			tabWidget->setTabSelected( tab );
			return true;
		}
		return false;
	}
	if ( request.action == AccessibilityAction::Press && widget->isType( UI_TYPE_PUSHBUTTON ) ) {
		static_cast<UIPushButton*>( widget )->onMouseClick( Vector2i(), EE_BUTTON_LMASK );
		return true;
	}
	if ( request.action == AccessibilityAction::Select && widget->isType( UI_TYPE_SELECTBUTTON ) ) {
		static_cast<UISelectButton*>( widget )->select();
		return true;
	}
	if ( request.action == AccessibilityAction::Toggle && widget->isType( UI_TYPE_CHECKBOX ) ) {
		auto checkbox = static_cast<UICheckBox*>( widget );
		checkbox->setChecked( !checkbox->isChecked() );
		return true;
	}
	if ( request.action == AccessibilityAction::Select && widget->isType( UI_TYPE_RADIOBUTTON ) ) {
		static_cast<UIRadioButton*>( widget )->setActive( true );
		return true;
	}
	if ( request.action == AccessibilityAction::SetText && widget->isType( UI_TYPE_TEXTINPUT ) ) {
		static_cast<UITextInput*>( widget )->setText( request.value );
		return true;
	}
	if ( request.action == AccessibilityAction::SetText && widget->isType( UI_TYPE_TEXTEDIT ) ) {
		static_cast<UITextEdit*>( widget )->setText( request.value );
		return true;
	}
	if ( request.action == AccessibilityAction::SetText && widget->isType( UI_TYPE_CODEEDITOR ) ) {
		// A code editor keeps its document (file path, undo history): replace its contents as
		// one undoable edit instead of resetting it.
		auto& document = static_cast<UICodeEditor*>( widget )->getDocument();
		document.selectAll();
		document.replaceSelection( request.value );
		return true;
	}
	if ( request.action == AccessibilityAction::SetTextSelection &&
		 ( widget->isType( UI_TYPE_TEXTINPUT ) || widget->isType( UI_TYPE_CODEEDITOR ) ) ) {
		Int32 start = 0;
		Int32 end = 0;
		if ( !parseSelection( request.value, start, end ) )
			return false;
		auto& document = widget->isType( UI_TYPE_CODEEDITOR )
							 ? static_cast<UICodeEditor*>( widget )->getDocument()
							 : static_cast<UITextInput*>( widget )->getDocument();
		document.setSelection( documentPosition( document, start ),
							   documentPosition( document, end ) );
		return true;
	}
	if ( ( request.action == AccessibilityAction::Expand ||
		   request.action == AccessibilityAction::Collapse ) &&
		 widget->isType( UI_TYPE_COMBOBOX ) ) {
		auto comboBox = static_cast<UIComboBox*>( widget );
		bool expanded = isComboBoxExpanded( comboBox );
		if ( ( request.action == AccessibilityAction::Expand ) != expanded )
			comboBox->getDropDownList()->showList();
		return true;
	}
	if ( widget->isType( UI_TYPE_SLIDER ) ) {
		auto slider = static_cast<UISlider*>( widget );
		if ( request.action == AccessibilityAction::Increment )
			slider->setValue( slider->getValue() + slider->getClickStep() );
		else if ( request.action == AccessibilityAction::Decrement )
			slider->setValue( slider->getValue() - slider->getClickStep() );
		else {
			Float value;
			if ( request.action != AccessibilityAction::SetValue ||
				 !String::fromString( value, request.value.toUtf8() ) )
				return false;
			slider->setValue( value );
		}
		return true;
	}
	if ( widget->isType( UI_TYPE_SPINBOX ) ) {
		auto spinBox = static_cast<UISpinBox*>( widget );
		if ( request.action == AccessibilityAction::Increment )
			spinBox->addValue( spinBox->getClickStep() );
		else if ( request.action == AccessibilityAction::Decrement )
			spinBox->addValue( -spinBox->getClickStep() );
		else {
			double value;
			if ( request.action != AccessibilityAction::SetValue ||
				 !String::fromString( value, request.value.toUtf8() ) )
				return false;
			spinBox->setValue( value );
		}
		return true;
	}
	return false;
}

}} // namespace EE::UI
