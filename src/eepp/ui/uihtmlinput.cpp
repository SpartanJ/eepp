#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uihtmltextinput.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

namespace {

static bool htmlBoolAttributeIsTrue( const StyleSheetProperty& property ) {
	if ( property.value().empty() )
		return true;

	const std::string& value = property.value();
	if ( String::iequals( value, property.getName() ) )
		return true;
	if ( String::iequals( value, "false" ) || value == "0" || String::iequals( value, "no" ) )
		return false;
	return property.asBool();
}

static std::string normalizeInputType( std::string type ) {
	type = String::toLower( String::trim( type ) );
	if ( type == "button" || type == "checkbox" || type == "color" || type == "date" ||
		 type == "datetime-local" || type == "email" || type == "file" || type == "hidden" ||
		 type == "image" || type == "month" || type == "number" || type == "password" ||
		 type == "radio" || type == "range" || type == "reset" || type == "search" ||
		 type == "submit" || type == "tel" || type == "text" || type == "time" || type == "url" ||
		 type == "week" )
		return type;
	return "text";
}

static bool isImplementationProperty( PropertyId id ) {
	switch ( id ) {
		case PropertyId::Size:
		case PropertyId::MaxLength:
		case PropertyId::AllowEditing:
		case PropertyId::Numeric:
		case PropertyId::AllowFloat:
		case PropertyId::InputMode:
		case PropertyId::Hint:
		case PropertyId::HintColor:
		case PropertyId::HintShadowColor:
		case PropertyId::HintShadowOffset:
		case PropertyId::HintFontFamily:
		case PropertyId::HintFontSize:
		case PropertyId::HintFontStyle:
		case PropertyId::HintStrokeWidth:
		case PropertyId::HintStrokeColor:
		case PropertyId::HintDisplay:
		case PropertyId::MinValue:
		case PropertyId::MaxValue:
		case PropertyId::ClickStep:
			return true;
		default:
			return false;
	}
}

static bool implementationPropertyAffectsIntrinsicSize( PropertyId id ) {
	switch ( id ) {
		case PropertyId::Size:
		case PropertyId::FontFamily:
		case PropertyId::FontSize:
		case PropertyId::FontStyle:
		case PropertyId::FontWeight:
			return true;
		default:
			return false;
	}
}

static void markAnonymousControlTree( Node* node ) {
	if ( node->isWidget() )
		node->asType<UIWidget>()->setFlags( UI_IGNORE_GLOBAL_CSS );
	for ( Node* child = node->getFirstChild(); child; child = child->getNextNode() )
		markAnonymousControlTree( child );
}

} // namespace

UIHTMLInput* UIHTMLInput::New() {
	return eeNew( UIHTMLInput, () );
}

UIHTMLInput::UIHTMLInput() : UIHTMLWidget( "input" ) {
	mFlags |= UI_HTML_ELEMENT;
	mDisplay = CSSDisplay::InlineBlock;
	mWidthPolicy = SizePolicy::WrapContent;
	mHeightPolicy = SizePolicy::WrapContent;
	createChildWidget();
}

Uint32 UIHTMLInput::getType() const {
	return UI_TYPE_HTML_INPUT;
}

bool UIHTMLInput::isType( const Uint32& type ) const {
	return UIHTMLInput::getType() == type || UIHTMLWidget::isType( type );
}

bool UIHTMLInput::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !attribute.getPropertyDefinition() )
		return false;

	PropertyId id = attribute.getPropertyDefinition()->getPropertyId();

	switch ( id ) {
		case PropertyId::Checked:
		case PropertyId::Selected:
			mChecked = htmlBoolAttributeIsTrue( attribute );
			syncCheckedState();
			return UIHTMLWidget::applyProperty( attribute );
		case PropertyId::Value:
		case PropertyId::Text:
			mValue = attribute.value();
			if ( mChildWidget && !( mInputType == "checkbox" || mInputType == "radio" ) )
				mChildWidget->applyProperty( attribute );
			return UIHTMLWidget::applyProperty( attribute );
		case PropertyId::Type:
			setInputType( attribute.value() );
			return true;
		default:
			break;
	}

	if ( isImplementationProperty( id ) || attribute.getPropertyDefinition()->isInherited() ) {
		mImplementationProperties[id] = attribute;
		if ( mChildWidget )
			applyImplementationProperty( attribute );
	}

	return UIHTMLWidget::applyProperty( attribute );
}

std::string UIHTMLInput::getPropertyString( const PropertyDefinition* propertyDef,
											const Uint32& propertyIndex ) const {
	if ( !propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Value:
			return mValue;
		case PropertyId::Checked:
		case PropertyId::Selected:
			return mChecked ? "true" : "false";
		case PropertyId::Type:
			return mInputType;
		default:
			break;
	}

	if ( mChildWidget ) {
		std::string val = mChildWidget->getPropertyString( propertyDef, propertyIndex );
		if ( !val.empty() )
			return val;
	}

	return UIHTMLWidget::getPropertyString( propertyDef, propertyIndex );
}

std::vector<PropertyId> UIHTMLInput::getPropertiesImplemented() const {
	auto props = UIHTMLWidget::getPropertiesImplemented();
	props.push_back( PropertyId::Type );
	props.push_back( PropertyId::Value );
	props.push_back( PropertyId::Checked );
	return props;
}

Float UIHTMLInput::getMinIntrinsicWidth() const {
	return mChildWidget ? mChildWidget->getMinIntrinsicWidth() : 0;
}

Float UIHTMLInput::getMaxIntrinsicWidth() const {
	return mChildWidget ? mChildWidget->getMaxIntrinsicWidth() : 0;
}

void UIHTMLInput::updateLayout() {
	// An input is a replaced element. Its anonymous native control must not be processed as a DOM
	// child by BlockLayouter, which would restore the control's intrinsic size after flex/grid had
	// assigned the host's final used size.
	positionOutOfFlowChildren();
	if ( isOutOfFlow() )
		updateOutOfFlowPosition();
	mDirtyLayout = false;
	updateChildGeometry();
}

const std::string& UIHTMLInput::getInputType() const {
	return mInputType;
}

void UIHTMLInput::setInputType( const std::string& type ) {
	const std::string normalizedType = normalizeInputType( type );
	if ( mInputType != normalizedType ) {
		syncStateFromImplementation();
		mInputType = normalizedType;
		createChildWidget();
	}
}

UIWidget* UIHTMLInput::getChildWidget() const {
	return mChildWidget;
}

void UIHTMLInput::createChildWidget() {
	if ( mChildWidget ) {
		mChildWidget->close();
		mChildWidget = nullptr;
	}

	if ( mInputType == "button" || mInputType == "submit" || mInputType == "reset" ) {
		mChildWidget = UIPushButton::New();
	} else if ( mInputType == "checkbox" ) {
		mChildWidget = UICheckBox::New();
	} else if ( mInputType == "hidden" ) {
		if ( !mHiddenByType ) {
			mVisibleBeforeHidden = isVisible();
			mEnabledBeforeHidden = isEnabled();
			mDisplayBeforeHidden = mDisplay;
		}
		mHiddenByType = true;
		setVisible( false );
		setEnabled( false );
		mDisplay = CSSDisplay::None;
	} else if ( mInputType == "number" ) {
		mChildWidget = UISpinBox::New();
	} else if ( mInputType == "password" ) {
		mChildWidget = UIHTMLTextInput::New()->setMode( UITextInput::TextInputMode::Password );
	} else if ( mInputType == "radio" ) {
		mChildWidget = UIRadioButton::New();
	} else {
		mChildWidget = UIHTMLTextInput::New();
	}

	if ( mChildWidget == nullptr )
		return;

	if ( mHiddenByType ) {
		mHiddenByType = false;
		mDisplay = mDisplayBeforeHidden;
		setEnabled( mEnabledBeforeHidden );
		setVisible( mVisibleBeforeHidden );
	}

	configureChildWidget();
	syncImplementationState();
}

void UIHTMLInput::configureChildWidget() {
	mChildWidget->setParent( this );
	markAnonymousControlTree( mChildWidget );
	mChildWidget->setLayoutWidthPolicy( SizePolicy::WrapContent );
	mChildWidget->setLayoutHeightPolicy( SizePolicy::WrapContent );

	// The child is anonymous control content, not a second HTML/CSS box. The host owns author
	// backgrounds, borders and padding; native subparts (check marks, spin buttons, etc.) remain.
	mChildWidget->removeSkin();
	mChildWidget->setBackgroundFillEnabled( false );
	mChildWidget->setBorderEnabled( false );
	mChildWidget->unsetFlags( UI_AUTO_PADDING );
	mChildWidget->setPadding( Rectf() );

	mChildWidget->on( Event::OnSizeChange, [this]( auto ) {
		if ( !mChildWidget || mSyncingGeometry )
			return;
		invalidateIntrinsicSize();
		updateHostGeometry();
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
	} );
}

void UIHTMLInput::applyImplementationProperty( const StyleSheetProperty& property ) {
	if ( !mChildWidget )
		return;

	const bool remeasure = implementationPropertyAffectsIntrinsicSize(
		property.getPropertyDefinition()->getPropertyId() );
	if ( remeasure ) {
		mChildWidget->setLayoutWidthPolicy( SizePolicy::WrapContent );
		mChildWidget->setLayoutHeightPolicy( SizePolicy::WrapContent );
	}
	mChildWidget->applyProperty( property );
	if ( remeasure ) {
		updateHostGeometry();
		mChildWidget->setLayoutWidthPolicy( SizePolicy::Fixed );
		mChildWidget->setLayoutHeightPolicy( SizePolicy::Fixed );
		updateChildGeometry();
	}
}

void UIHTMLInput::syncImplementationState() {
	if ( !mChildWidget )
		return;

	for ( const auto& prop : mImplementationProperties )
		mChildWidget->applyProperty( prop.second );

	if ( !( mInputType == "checkbox" || mInputType == "radio" ) )
		mChildWidget->applyProperty( StyleSheetProperty( "value", mValue ) );
	syncCheckedState();
	updateHostGeometry();
	mChildWidget->setLayoutWidthPolicy( SizePolicy::Fixed );
	mChildWidget->setLayoutHeightPolicy( SizePolicy::Fixed );
	updateChildGeometry();
}

void UIHTMLInput::syncStateFromImplementation() {
	if ( !mChildWidget )
		return;

	if ( mInputType == "checkbox" ) {
		mChecked = static_cast<UICheckBox*>( mChildWidget )->isChecked();
	} else if ( mInputType == "radio" ) {
		mChecked = static_cast<UIRadioButton*>( mChildWidget )->isActive();
	} else {
		mValue = getFormValue();
	}
}

void UIHTMLInput::updateHostGeometry() {
	if ( !mChildWidget || mSyncingGeometry )
		return;

	const Rectf contentOffset = getPixelsContentOffset();
	Sizef size = getPixelsSize();
	if ( getLayoutWidthPolicy() == SizePolicy::WrapContent )
		size.setWidth( mChildWidget->getPixelsSize().getWidth() + contentOffset.Left +
					   contentOffset.Right );
	if ( getLayoutHeightPolicy() == SizePolicy::WrapContent )
		size.setHeight( mChildWidget->getPixelsSize().getHeight() + contentOffset.Top +
						contentOffset.Bottom );

	mSyncingGeometry = true;
	setPixelsSize( size );
	mSyncingGeometry = false;
	updateChildGeometry();
}

void UIHTMLInput::updateChildGeometry() {
	if ( !mChildWidget || mSyncingGeometry )
		return;

	const Rectf contentOffset = getPixelsContentOffset();
	const Sizef contentSize(
		eemax( 0.f, getPixelsSize().getWidth() - contentOffset.Left - contentOffset.Right ),
		eemax( 0.f, getPixelsSize().getHeight() - contentOffset.Top - contentOffset.Bottom ) );
	mSyncingGeometry = true;
	mChildWidget->setPixelsPosition( contentOffset.Left, contentOffset.Top );
	if ( contentSize.getWidth() > 0 && contentSize.getHeight() > 0 )
		mChildWidget->setPixelsSize( contentSize );
	mSyncingGeometry = false;
}

void UIHTMLInput::syncCheckedState() {
	if ( !mChildWidget )
		return;

	if ( mInputType == "checkbox" ) {
		static_cast<UICheckBox*>( mChildWidget )->setChecked( mChecked );
	} else if ( mInputType == "radio" ) {
		static_cast<UIRadioButton*>( mChildWidget )->setActive( mChecked );
	}
}

String UIHTMLInput::getFormValue() const {
	if ( !mChildWidget )
		return String();

	if ( mInputType == "checkbox" )
		return static_cast<UICheckBox*>( mChildWidget )->isChecked()
				   ? ( mValue.empty() ? "on" : mValue )
				   : "";
	if ( mInputType == "radio" )
		return static_cast<UIRadioButton*>( mChildWidget )->isActive()
				   ? ( mValue.empty() ? "on" : mValue )
				   : "";
	if ( mInputType == "number" )
		return static_cast<UISpinBox*>( mChildWidget )->getTextInput()->getText();
	if ( mInputType == "button" || mInputType == "submit" )
		return static_cast<UIPushButton*>( mChildWidget )->getText();

	if ( mChildWidget->isType( UI_TYPE_TEXTINPUT ) )
		return static_cast<UITextInput*>( mChildWidget )->getText();

	return mValue;
}

void UIHTMLInput::onSizeChange() {
	UIHTMLWidget::onSizeChange();
	updateChildGeometry();
}

void UIHTMLInput::onPaddingChange() {
	UIHTMLWidget::onPaddingChange();
	updateHostGeometry();
}

}} // namespace EE::UI
