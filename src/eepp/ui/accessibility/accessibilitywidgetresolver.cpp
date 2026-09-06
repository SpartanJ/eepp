#include <eepp/ui/accessibility/accessibilitywidgetresolver.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenuradiobutton.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI {

namespace {

bool isRoot( const UIWidget* widget ) {
	return widget->getUISceneNode() && widget == widget->getUISceneNode()->getRoot();
}

AccessibilityState baseState( const UIWidget* widget ) {
	AccessibilityState state = AccessibilityState::None;
	if ( widget->isEnabled() )
		state |= AccessibilityState::Enabled;
	if ( widget->isTabFocusable() )
		state |= AccessibilityState::Focusable;
	if ( widget->hasFocus() )
		state |= AccessibilityState::Focused;
	if ( widget->isVisible() )
		state |= AccessibilityState::Visible;
	if ( widget->hasVisibility() )
		state |= AccessibilityState::Showing;
	return state;
}

AccessibilityActions baseActions( const UIWidget* widget ) {
	return widget->isTabFocusable() ? accessibilityActionMask( AccessibilityAction::Focus ) : 0;
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

String AccessibilityWidgetResolver::getName( const UIWidget* widget ) {
	String name;
	if ( isRoot( widget ) && widget->getUISceneNode()->getWindow() )
		name = String::fromUtf8( widget->getUISceneNode()->getWindow()->getTitle() );
	else if ( widget->isType( UI_TYPE_PUSHBUTTON ) )
		name = static_cast<const UIPushButton*>( widget )->getText();
	else if ( widget->isType( UI_TYPE_TEXTVIEW ) )
		name = static_cast<const UITextView*>( widget )->getText();
	return widget->resolveAccessibilityName( name );
}

String AccessibilityWidgetResolver::getDescription( const UIWidget* widget ) {
	return widget->resolveAccessibilityDescription();
}

String AccessibilityWidgetResolver::getValue( const UIWidget* widget ) {
	if ( widget->isType( UI_TYPE_TEXTINPUT ) )
		return static_cast<const UITextInput*>( widget )->getText();
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

AccessibilityState AccessibilityWidgetResolver::getState( const UIWidget* widget ) {
	auto state = baseState( widget );
	if ( widget->isType( UI_TYPE_CHECKBOX ) &&
		 static_cast<const UICheckBox*>( widget )->isChecked() )
		state |= AccessibilityState::Checked;
	if ( widget->isType( UI_TYPE_RADIOBUTTON ) &&
		 static_cast<const UIRadioButton*>( widget )->isActive() )
		state |= AccessibilityState::Selected;
	if ( widget->isType( UI_TYPE_SELECTBUTTON ) &&
		 static_cast<const UISelectButton*>( widget )->isSelected() )
		state |= AccessibilityState::Selected;
	if ( widget->isType( UI_TYPE_TEXTINPUT ) )
		state |= AccessibilityState::Editable;
	return state;
}

AccessibilityActions AccessibilityWidgetResolver::getActions( const UIWidget* widget ) {
	if ( isRoot( widget ) ||
		 ( widget->isType( UI_TYPE_TEXTVIEW ) && !widget->isType( UI_TYPE_CHECKBOX ) &&
		   !widget->isType( UI_TYPE_RADIOBUTTON ) && !widget->isType( UI_TYPE_TEXTINPUT ) ) )
		return 0;
	auto actions = baseActions( widget );
	if ( widget->isType( UI_TYPE_PUSHBUTTON ) )
		actions |= accessibilityActionMask( AccessibilityAction::Press );
	if ( widget->isType( UI_TYPE_SELECTBUTTON ) )
		actions |= accessibilityActionMask( AccessibilityAction::Select );
	if ( widget->isType( UI_TYPE_CHECKBOX ) )
		actions |= accessibilityActionMask( AccessibilityAction::Toggle );
	if ( widget->isType( UI_TYPE_RADIOBUTTON ) )
		actions |= accessibilityActionMask( AccessibilityAction::Select );
	if ( widget->isType( UI_TYPE_TEXTINPUT ) )
		actions |= accessibilityActionMask( AccessibilityAction::SetText );
	if ( widget->isType( UI_TYPE_SLIDER ) || widget->isType( UI_TYPE_SPINBOX ) )
		actions |= accessibilityActionMask( AccessibilityAction::Increment ) |
				   accessibilityActionMask( AccessibilityAction::Decrement ) |
				   accessibilityActionMask( AccessibilityAction::SetValue );
	return actions;
}

bool AccessibilityWidgetResolver::performAction( UIWidget* widget,
												 const AccessibilityActionRequest& request ) {
	if ( !( getActions( widget ) & accessibilityActionMask( request.action ) ) )
		return false;
	if ( request.action == AccessibilityAction::Focus ) {
		widget->setFocus( NodeFocusReason::Unknown );
		return true;
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
