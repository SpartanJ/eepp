#include <charconv>
#include <cmath>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uidatepicker.hpp>
#include <eepp/ui/uidatetimepicker.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uihtmltextinput.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitimepicker.hpp>
#include <limits>

namespace EE { namespace UI {

using namespace EE::System;

namespace {

static void markAnonymousControlTree( Node* node ) {
	if ( node->isWidget() )
		node->asType<UIWidget>()->setFlags( UI_IGNORE_GLOBAL_CSS );
	for ( Node* child = node->getFirstChild(); child; child = child->getNextNode() )
		markAnonymousControlTree( child );
}

// HTML temporal values have stricter syntax than the native editor's tolerant paste parser.
// https://html.spec.whatwg.org/multipage/common-microsyntaxes.html#dates-and-times
static std::optional<CalendarDate> parseHTMLDate( std::string_view text ) {
	const size_t dash = text.find( '-' );
	if ( dash < 4 || dash == std::string_view::npos || text.size() != dash + 6 ||
		 text[dash + 3] != '-' )
		return std::nullopt;
	for ( size_t i = 0; i < text.size(); ++i ) {
		if ( i != dash && i != dash + 3 && ( text[i] < '0' || text[i] > '9' ) )
			return std::nullopt;
	}
	Int32 year;
	const auto parsed = std::from_chars( text.data(), text.data() + dash, year );
	if ( parsed.ec != std::errc() || year < 1 )
		return std::nullopt;
	const auto pair = [&]( size_t pos ) { return ( text[pos] - '0' ) * 10 + text[pos + 1] - '0'; };
	CalendarDate value{ year, static_cast<Uint8>( pair( dash + 1 ) ),
						static_cast<Uint8>( pair( dash + 4 ) ) };
	return value.isValid() ? std::optional<CalendarDate>( value ) : std::nullopt;
}

static std::optional<TimeOfDay> parseHTMLTime( std::string_view text ) {
	if ( !( text.size() == 5 || text.size() == 8 || ( text.size() >= 10 && text.size() <= 12 ) ) ||
		 text[2] != ':' || ( text.size() > 5 && text[5] != ':' ) ||
		 ( text.size() > 8 && text[8] != '.' ) )
		return std::nullopt;
	for ( size_t i = 0; i < text.size(); ++i ) {
		if ( i != 2 && i != 5 && i != 8 && ( text[i] < '0' || text[i] > '9' ) )
			return std::nullopt;
	}
	const auto pair = [&]( size_t pos ) { return ( text[pos] - '0' ) * 10 + text[pos + 1] - '0'; };
	Uint16 ms = 0;
	for ( size_t i = 9; i < text.size(); ++i )
		ms = ms * 10 + text[i] - '0';
	for ( size_t i = text.size(); i > 8 && i < 12; ++i )
		ms *= 10;
	TimeOfDay value{ static_cast<Uint8>( pair( 0 ) ), static_cast<Uint8>( pair( 3 ) ),
					 static_cast<Uint8>( text.size() > 5 ? pair( 6 ) : 0 ), ms };
	return value.isValid() ? std::optional<TimeOfDay>( value ) : std::nullopt;
}

static std::optional<LocalDateTime> parseHTMLTemporal( std::string_view text,
													   const std::string& type ) {
	if ( type == "date" ) {
		if ( auto date = parseHTMLDate( text ) )
			return LocalDateTime{ *date, {} };
	} else if ( type == "time" ) {
		if ( auto time = parseHTMLTime( text ) )
			return LocalDateTime{ {}, *time };
	} else {
		const size_t separator = text.find_first_of( "T " );
		if ( separator != std::string_view::npos ) {
			const auto date = parseHTMLDate( text.substr( 0, separator ) );
			const auto time = parseHTMLTime( text.substr( separator + 1 ) );
			if ( date && time )
				return LocalDateTime{ *date, *time };
		}
	}
	return std::nullopt;
}

static std::string formatHTMLTime( const TimeOfDay& time ) {
	std::string value = DateTimeFormatter::toISOTime( time, time.millisecond != 0 );
	if ( time.millisecond ) {
		while ( value.back() == '0' )
			value.pop_back();
	} else if ( !time.second ) {
		value.resize( 5 );
	}
	return value;
}

static long double htmlTemporalNumber( const LocalDateTime& value ) {
	return static_cast<long double>( value.date.daysSinceEpoch() ) * 86400000.L +
		   ( value.time.hour * 3600 + value.time.minute * 60 + value.time.second ) * 1000.L +
		   value.time.millisecond;
}

static double htmlTemporalStep( const std::string& text, bool date ) {
	double step;
	const auto parsed = std::from_chars( text.data(), text.data() + text.size(), step );
	return parsed.ec == std::errc() && parsed.ptr == text.data() + text.size() &&
				   std::isfinite( step ) && step > 0
			   ? step
			   : ( date ? 1. : 60. );
}

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
		case PropertyId::Height:
			mHasSpecifiedHeight = !attribute.value().empty() &&
								  !String::iequals( String::trim( attribute.value() ), "auto" );
			break;
		case PropertyId::Selected:
			mChecked = htmlBoolAttributeIsTrue( attribute );
			syncCheckedState();
			return UIHTMLWidget::applyProperty( attribute );
		case PropertyId::Value:
		case PropertyId::Text:
			mImplementationProperties[PropertyId::Value] = attribute;
			if ( isTemporalInput() ) {
				setTemporalValue( attribute.value() );
				syncTemporalConfiguration();
				return true;
			}
			mValue = attribute.value();
			if ( mChildWidget && !( mInputType == "checkbox" || mInputType == "radio" ) )
				mChildWidget->applyProperty( attribute );
			return UIHTMLWidget::applyProperty( attribute );
		case PropertyId::Min:
		case PropertyId::Max:
		case PropertyId::Step:
		case PropertyId::Required:
		case PropertyId::ReadOnly:
		case PropertyId::Disabled:
			mImplementationProperties[id] = attribute;
			if ( id == PropertyId::Disabled )
				setEnabled( false );
			if ( isTemporalInput() )
				syncTemporalConfiguration();
			else if ( id == PropertyId::ReadOnly && mChildWidget &&
					  mChildWidget->isType( UI_TYPE_TEXTINPUT ) )
				mChildWidget->asType<UITextInput>()->setAllowEditing( false );
			return true;
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
			return isTemporalInput() ? getFormValue().toUtf8() : mValue.toUtf8();
		case PropertyId::Selected:
			return mChecked ? "true" : "false";
		case PropertyId::Type:
			return mInputType;
		default:
			break;
	}

	if ( auto it = mImplementationProperties.find( propertyDef->getPropertyId() );
		 it != mImplementationProperties.end() &&
		 ( propertyDef->getPropertyId() == PropertyId::Min ||
		   propertyDef->getPropertyId() == PropertyId::Max ||
		   propertyDef->getPropertyId() == PropertyId::Step ||
		   propertyDef->getPropertyId() == PropertyId::Required ||
		   propertyDef->getPropertyId() == PropertyId::ReadOnly ||
		   propertyDef->getPropertyId() == PropertyId::Disabled ) )
		return it->second.value();

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
	for ( const auto id : { PropertyId::Min, PropertyId::Max, PropertyId::Step,
							PropertyId::Required, PropertyId::ReadOnly, PropertyId::Disabled } )
		props.push_back( id );
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
	if ( !mHasSpecifiedHeight )
		updateHostGeometry();
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
	mControlChildrenConnection.disconnect();
	mPopupTimeChildrenConnection.disconnect();
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
	} else if ( mInputType == "date" ) {
		mChildWidget = UIDatePicker::New();
	} else if ( mInputType == "time" ) {
		mChildWidget = UITimePicker::New();
	} else if ( mInputType == "datetime-local" ) {
		mChildWidget = UIDateTimePicker::New();
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
	if ( isTemporalInput() ) {
		// Anonymous popups remain in the document scene and use its private UA defaults.
		mControlChildrenConnection =
			mChildWidget->connect( Event::OnChildCountChanged, [this]( const Event* event ) {
				const auto* change = static_cast<const ChildCountChangedEvent*>( event );
				if ( change->removed() )
					return;
				markAnonymousControlTree( change->child() );
				if ( auto* time = change->child()->findByType( UI_TYPE_TIMEPICKER ) ) {
					// Seconds and milliseconds can create new arrows after the popup is open.
					mPopupTimeChildrenConnection =
						time->connect( Event::OnChildCountChanged, []( const Event* event ) {
							const auto* change =
								static_cast<const ChildCountChangedEvent*>( event );
							if ( !change->removed() )
								markAnonymousControlTree( change->child() );
						} );
				}
			} );
		static_cast<UIDateTimeEdit*>( mChildWidget )->setClampToBounds( false );
	}
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

	for ( const auto& [id, property] : mImplementationProperties ) {
		if ( id != PropertyId::Value && id != PropertyId::Min && id != PropertyId::Max &&
			 id != PropertyId::Step && id != PropertyId::Required && id != PropertyId::ReadOnly &&
			 id != PropertyId::Disabled )
			mChildWidget->applyProperty( property );
	}

	if ( isTemporalInput() ) {
		setTemporalValue( mValue );
		syncTemporalConfiguration();
	} else if ( !( mInputType == "checkbox" || mInputType == "radio" ) ) {
		mChildWidget->applyProperty( StyleSheetProperty( "value", mValue ) );
		if ( mImplementationProperties.count( PropertyId::ReadOnly ) &&
			 mChildWidget->isType( UI_TYPE_TEXTINPUT ) ) {
			mChildWidget->asType<UITextInput>()->setAllowEditing( false );
		}
	}
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
	if ( getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
		// Temporal editors may already have a fixed used size when their font or value changes.
		const Float width = isTemporalInput() ? mChildWidget->getMaxIntrinsicWidth()
											  : mChildWidget->getPixelsSize().getWidth();
		size.setWidth( width + contentOffset.Left + contentOffset.Right );
	}
	if ( getLayoutHeightPolicy() == SizePolicy::WrapContent || !mHasSpecifiedHeight ) {
		Float contentHeight = mChildWidget->getPixelsSize().getHeight();
		if ( mChildWidget->isType( UI_TYPE_HTML_TEXTINPUT ) )
			contentHeight = mChildWidget->asType<UIHTMLTextInput>()->getIntrinsicContentHeight();
		size.setHeight( contentHeight + contentOffset.Top + contentOffset.Bottom );
	}

	mSyncingGeometry = true;
	setPixelsSize( size );
	mSyncingGeometry = false;
	updateChildGeometry();
}

Float UIHTMLInput::getReplacedElementBaseline() const {
	if ( mChildWidget && mChildWidget->isType( UI_TYPE_HTML_TEXTINPUT ) ) {
		return getPixelsContentOffset().Top +
			   mChildWidget->asType<UIHTMLTextInput>()->getTextBaseline();
	}
	if ( isTemporalInput() && mChildWidget ) {
		const auto* edit = static_cast<UIDateTimeEdit*>( mChildWidget );
		if ( edit->getFont() ) {
			return getPixelsContentOffset().Top + edit->getPixelsPadding().Top +
				   edit->getRealAlignOffset().y + edit->getFont()->getAscent( edit->getFontSize() );
		}
	}
	return getPixelsSize().getHeight();
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

	if ( isTemporalInput() ) {
		auto* edit = static_cast<UIDateTimeEdit*>( mChildWidget );
		const auto rawValue = parseHTMLTemporal( mValue.toUtf8(), mInputType );
		if ( mInputType != "datetime-local" && rawValue && rawValue == getTemporalValue() )
			return mValue;
		if ( mInputType == "date" ) {
			return edit->getDate() && edit->getDate()->year > 0
					   ? DateTimeFormatter::toISODate( *edit->getDate() )
					   : "";
		}
		if ( mInputType == "time" )
			return edit->getTime() ? formatHTMLTime( *edit->getTime() ) : "";
		return edit->getDateTime() && edit->getDateTime()->date.year > 0
				   ? DateTimeFormatter::toISODate( edit->getDateTime()->date ) + "T" +
						 formatHTMLTime( edit->getDateTime()->time )
				   : "";
	}

	if ( mChildWidget->isType( UI_TYPE_TEXTINPUT ) )
		return static_cast<UITextInput*>( mChildWidget )->getText();

	return mValue;
}

bool UIHTMLInput::isTemporalInput() const {
	return mInputType == "date" || mInputType == "time" || mInputType == "datetime-local";
}

const std::string& UIHTMLInput::temporalAttribute( PropertyId id ) const {
	static const std::string empty;
	const auto it = mImplementationProperties.find( id );
	return it == mImplementationProperties.end() ? empty : it->second.value();
}

std::optional<LocalDateTime> UIHTMLInput::getTemporalValue() const {
	if ( !mChildWidget || !isTemporalInput() )
		return std::nullopt;
	const auto* edit = static_cast<const UIDateTimeEdit*>( mChildWidget );
	if ( mInputType == "date" ) {
		if ( edit->getDate() && edit->getDate()->year > 0 )
			return LocalDateTime{ *edit->getDate(), {} };
	} else if ( mInputType == "time" ) {
		if ( edit->getTime() )
			return LocalDateTime{ {}, *edit->getTime() };
	} else if ( edit->getDateTime() && edit->getDateTime()->date.year > 0 ) {
		return edit->getDateTime();
	}
	return std::nullopt;
}

void UIHTMLInput::setTemporalValue( const String& text ) {
	const auto value = parseHTMLTemporal( text.toUtf8(), mInputType );
	auto* edit = static_cast<UIDateTimeEdit*>( mChildWidget );
	if ( mInputType == "date" )
		edit->setDate( value ? std::optional<CalendarDate>( value->date ) : std::nullopt );
	else if ( mInputType == "time" )
		edit->setTime( value ? std::optional<TimeOfDay>( value->time ) : std::nullopt );
	else
		edit->setDateTime( value );
	mValue = value ? text : String();
	if ( mInputType == "datetime-local" )
		mValue = getFormValue();
}

void UIHTMLInput::syncTemporalConfiguration() {
	auto* edit = static_cast<UIDateTimeEdit*>( mChildWidget );
	const auto minimum = parseHTMLTemporal( temporalAttribute( PropertyId::Min ), mInputType );
	const auto maximum = parseHTMLTemporal( temporalAttribute( PropertyId::Max ), mInputType );
	// HTML ranges validate without clamping. Native reversed time bounds are not periodic, so
	// leave those bounds open and validate the wrapping interval here instead.
	if ( mInputType == "date" ) {
		edit->setMinDate( std::nullopt );
		edit->setMaxDate( std::nullopt );
		if ( !minimum || !maximum || *minimum <= *maximum ) {
			edit->setMinDate( minimum ? minimum->date : CalendarDate{ 1, 1, 1 } );
			edit->setMaxDate( maximum ? std::optional<CalendarDate>( maximum->date )
									  : std::nullopt );
		}
	} else if ( mInputType == "time" ) {
		edit->setMinTime( std::nullopt );
		edit->setMaxTime( std::nullopt );
		if ( !minimum || !maximum || *minimum <= *maximum ) {
			edit->setMinTime( minimum ? std::optional<TimeOfDay>( minimum->time ) : std::nullopt );
			edit->setMaxTime( maximum ? std::optional<TimeOfDay>( maximum->time ) : std::nullopt );
		}
	} else {
		edit->setMinDateTime( std::nullopt );
		edit->setMaxDateTime( std::nullopt );
		if ( !minimum || !maximum || *minimum <= *maximum ) {
			edit->setMinDateTime( minimum.value_or( LocalDateTime{ { 1, 1, 1 }, {} } ) );
			edit->setMaxDateTime( maximum );
		}
	}
	const auto allowEditing = mImplementationProperties.find( PropertyId::AllowEditing );
	edit->setAllowEditing(
		mImplementationProperties.count( PropertyId::ReadOnly ) == 0 &&
		( allowEditing == mImplementationProperties.end() || allowEditing->second.asBool() ) );
	const double step =
		htmlTemporalStep( temporalAttribute( PropertyId::Step ), mInputType == "date" );
	const auto nativeStep = []( double value ) {
		return static_cast<Uint32>(
			std::clamp( value, 1., static_cast<double>( std::numeric_limits<Uint32>::max() ) ) );
	};
	edit->setDayStep( nativeStep( step ) );
	edit->setMinuteStep( std::fmod( step, 60. ) == 0 ? nativeStep( step / 60. ) : 1 );
	edit->setSecondStep( nativeStep( step ) );
	const auto value = getTemporalValue();
	const bool milliseconds =
		( value && value->time.millisecond ) || ( minimum && minimum->time.millisecond ) ||
		( maximum && maximum->time.millisecond ) || std::floor( step ) != step;
	const bool seconds = milliseconds || ( value && value->time.second ) ||
						 ( minimum && minimum->time.second ) ||
						 ( maximum && maximum->time.second ) || std::fmod( step, 60. ) != 0;
	edit->setShowMilliseconds( milliseconds );
	edit->setShowSeconds( seconds );
}

bool UIHTMLInput::willValidate() const {
	return isTemporalInput() && isEnabled() &&
		   mImplementationProperties.count( PropertyId::ReadOnly ) == 0;
}

HTMLInputValidity UIHTMLInput::getValidity() const {
	HTMLInputValidity validity;
	if ( !isTemporalInput() || !mChildWidget )
		return validity;
	const auto value = getTemporalValue();
	validity.valueMissing = !value &&
							mImplementationProperties.count( PropertyId::Required ) != 0 &&
							mImplementationProperties.count( PropertyId::ReadOnly ) == 0;
	validity.badInput = !value && static_cast<UIDateTimeEdit*>( mChildWidget )->hasValue();
	if ( !value )
		return validity;
	const auto minimum = parseHTMLTemporal( temporalAttribute( PropertyId::Min ), mInputType );
	const auto maximum = parseHTMLTemporal( temporalAttribute( PropertyId::Max ), mInputType );
	validity.rangeUnderflow = minimum && *value < *minimum;
	validity.rangeOverflow = maximum && *value > *maximum;
	if ( mInputType == "time" && minimum && maximum && *minimum > *maximum ) {
		validity.rangeUnderflow = validity.rangeOverflow = *value > *maximum && *value < *minimum;
	}
	const std::string& stepText = temporalAttribute( PropertyId::Step );
	if ( !String::iequals( stepText, "any" ) ) {
		const auto initial =
			parseHTMLTemporal( temporalAttribute( PropertyId::Value ), mInputType );
		const long double base = minimum   ? htmlTemporalNumber( *minimum )
								 : initial ? htmlTemporalNumber( *initial )
										   : 0.L;
		const long double step = htmlTemporalStep( stepText, mInputType == "date" ) *
								 ( mInputType == "date" ? 86400000.L : 1000.L );
		const long double offset = htmlTemporalNumber( *value ) - base;
		const long double remainder = std::abs( std::remainder( offset, step ) );
		validity.stepMismatch =
			remainder > std::max( 0.000001L, std::abs( offset ) *
												 std::numeric_limits<long double>::epsilon() * 8 );
	}
	return validity;
}

bool UIHTMLInput::checkValidity() const {
	return !willValidate() || getValidity().valid();
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
