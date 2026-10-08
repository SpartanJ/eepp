#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uicalendar.hpp>
#include <eepp/ui/uidatepicker.hpp>
#include <eepp/ui/uipopup.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

using namespace System;
using namespace CSS;

UIDatePicker* UIDatePicker::New() {
	return eeNew( UIDatePicker, () );
}

UIDatePicker::UIDatePicker() : UIDatePicker( DateTimeEditMode::Date, "datepicker" ) {}

UIDatePicker::UIDatePicker( DateTimeEditMode mode, const std::string& tag ) :
	UIDateTimeEdit( mode, tag ), mEditorRightPadding( getPadding().Right ) {
	setClipType( ClipType::BorderBox );
	mCalendarButton = UIPushButton::NewWithTag( tag + "::button" );
	// The editor owns the button's hit area; its CSS image must not collapse to an empty label.
	mCalendarButton->unsetFlags( UI_AUTO_SIZE );
	mCalendarButton->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mCalendarButton->setParent( this );
	mConnections[0] = mCalendarButton->connect( Event::MouseClick, [this]( const Event* event ) {
		if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) )
			return;
		if ( isCalendarVisible() )
			hideCalendar();
		else
			showCalendar();
	} );
	mConnections[1] =
		mCalendarButton->connect( Event::OnSizeChange, [this]( const Event* ) { updateButton(); } );
	mConnections[2] = connect( Event::OnUpdateScreenPosition, [this]( const Event* ) {
		if ( mCalendarVisible )
			UIPopUp::align( this, getCalendarPopup(), mPopUpToRoot );
	} );
	mConnections[3] =
		connect( Event::OnFocusWithinLoss, [this]( const Event* ) { closeOnFocusLoss(); } );
	updateButton();
}

UIDatePicker::~UIDatePicker() {
	mCalendarVisible = false;
	mConnections = {};
	mCalendarConnections = {};
	// The calendar can outlive its original parent after popup alignment reparents it.
	if ( mCalendar ) {
		auto* calendar = mCalendar;
		mCalendar = nullptr;
		// A closing calendar already belongs to the scene's deferred deletion queue.
		if ( !calendar->isClosing() )
			eeDelete( calendar );
	}
}

Uint32 UIDatePicker::getType() const {
	return UI_TYPE_DATEPICKER;
}

bool UIDatePicker::isType( const Uint32& type ) const {
	return type == UI_TYPE_DATEPICKER || UIDateTimeEdit::isType( type );
}

Float UIDatePicker::getMinIntrinsicWidth() const {
	// Reserve the square button's full width even before the editor has its first used width.
	return UIDateTimeEdit::getMinIntrinsicWidth() - PixelDensity::dpToPx( mReservedButtonWidth ) +
		   getPixelsSize().getHeight();
}

UIDatePicker* UIDatePicker::showCalendar() {
	if ( !isEnabled() || !isVisible() || !isEditingAllowed() || !getUISceneNode() ||
		 getEditMode() == DateTimeEditMode::Time )
		return this;
	auto* popup = getCalendarPopup();
	if ( ( mCalendar && mCalendar->isClosing() ) || ( popup && popup->isClosing() ) )
		return this;
	commitPendingDigits( false );
	if ( !mCalendar ) {
		mCalendar = UICalendar::New();
		mCalendar->setVisible( false );
		mCalendar->setEnabled( false );
		mCalendar->setParent( this );
		mCalendarConnections[0] = mCalendar->connect( Event::OnClose, [this]( const Event* ) {
			auto* popup = getCalendarPopup();
			if ( popup != mCalendar )
				UIPopUp::hide( popup );
			mCalendar = nullptr;
			mCalendarVisible = false;
			mSceneSizeConnection.disconnect();
		} );
		mCalendarConnections[1] = mCalendar->connect( Event::OnSizeChange, [this]( const Event* ) {
			if ( mCalendarVisible )
				UIPopUp::align( this, getCalendarPopup(), mPopUpToRoot );
		} );
		mCalendarConnections[2] = mCalendar->connect(
			Event::OnFocusedDateChange, [this]( const Event* ) { onCalendarFocusedDateChange(); } );
		mCalendarConnections[3] = mCalendar->connect(
			Event::OnItemSelected, [this]( const Event* ) { selectCalendarDate(); } );
		mCalendarConnections[4] = mCalendar->connect(
			Event::OnFocusWithinLoss, [this]( const Event* ) { closeOnFocusLoss(); } );
		mCalendarConnections[5] = mCalendar->connect( Event::KeyDown, [this]( const Event* event ) {
			if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
				hideCalendar();
		} );
	}
	mCalendar->setLocale( mLocale );
	UIDatePicker::onConstraintsChange();
	const std::optional<CalendarDate> selected =
		getEditMode() == DateTimeEditMode::DateTime
			? ( getDateTime() ? std::optional<CalendarDate>( getDateTime()->date ) : std::nullopt )
			: getDate();
	mCalendar->setSelectedDate( selected );
	mCalendar->setFocusedDate( selected.value_or( CalendarDate::today() ) );
	mCalendar->setView( CalendarView::Days );
	prepareCalendarPopup();
	UIPopUp::align( this, getCalendarPopup(), mPopUpToRoot );
	mCalendarVisible = true;
	mSceneSizeConnection = getUISceneNode()->connect( Event::OnSizeChange, [this]( const Event* ) {
		if ( mCalendarVisible )
			UIPopUp::align( this, getCalendarPopup(), mPopUpToRoot );
	} );
	UIPopUp::show( getCalendarPopup() );
	mCalendar->setFocus();
	return this;
}

UIDatePicker* UIDatePicker::hideCalendar( bool restoreFocus ) {
	if ( !mCalendarVisible )
		return this;
	mCalendarVisible = false;
	mSceneSizeConnection.disconnect();
	UIPopUp::hide( getCalendarPopup() );
	if ( restoreFocus && isEnabled() && isVisible() && !isClosing() )
		setFocus();
	return this;
}

bool UIDatePicker::isCalendarVisible() const {
	return mCalendarVisible;
}

UICalendar* UIDatePicker::getCalendar() const {
	return mCalendar;
}

UIPushButton* UIDatePicker::getCalendarButton() const {
	return mCalendarButton;
}

UIWidget* UIDatePicker::getCalendarPopup() const {
	return mCalendar;
}

void UIDatePicker::prepareCalendarPopup() {}

void UIDatePicker::onConstraintsChange() {
	if ( !mCalendar )
		return;
	// Reset old constraints first so changing a range does not inherit a previous popup bound.
	mCalendar->setMinDate( std::nullopt );
	mCalendar->setMaxDate( std::nullopt );
	auto minimum = getMinDate();
	auto maximum = getMaxDate();
	if ( getEditMode() == DateTimeEditMode::DateTime ) {
		if ( getMinDateTime() )
			minimum = getMinDateTime()->date;
		if ( getMaxDateTime() )
			maximum = getMaxDateTime()->date;
	}
	mCalendar->setMinDate( minimum );
	mCalendar->setMaxDate( maximum );
}

void UIDatePicker::onCalendarFocusedDateChange() {}

void UIDatePicker::onConfigurationChange() {
	if ( mCalendar && mCalendar->getLocale() != getLocale() )
		mCalendar->setLocale( getLocale() );
	if ( !isEditingAllowed() )
		hideCalendar( false );
}

void UIDatePicker::selectCalendarDate() {
	if ( !mCalendar || !mCalendar->getSelectedDate() || !isEditingAllowed() )
		return;
	const CalendarDate date = *mCalendar->getSelectedDate();
	if ( getEditMode() == DateTimeEditMode::DateTime )
		setDateTime( LocalDateTime{ date, getDateTime() ? getDateTime()->time : TimeOfDay{} } );
	else
		setDate( date );
	hideCalendar();
}

void UIDatePicker::closeOnFocusLoss() {
	if ( mCalendarVisible && !UIPopUp::hasFocus( this, getCalendarPopup() ) )
		hideCalendar( false );
}

Uint32 UIDatePicker::onKeyDown( const KeyEvent& event ) {
	if ( event.getKeyCode() == KEY_ESCAPE && mCalendarVisible ) {
		hideCalendar();
		return 1;
	}
	if ( event.getKeyCode() == KEY_F4 ||
		 ( event.getKeyCode() == KEY_DOWN && ( event.getSanitizedMod() & KEYMOD_ALT ) ) ) {
		if ( mCalendarVisible )
			hideCalendar();
		else
			showCalendar();
		return 1;
	}
	return UIDateTimeEdit::onKeyDown( event );
}

void UIDatePicker::draw() {
	// Clip only the inherited text drawing; the trailing child button needs the full field box.
	clipSmartEnable( mScreenPos.x + mPaddingPx.Left, mScreenPos.y + mPaddingPx.Top,
					 eemax( 0.f, mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right ),
					 eemax( 0.f, mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom ) );
	UIDateTimeEdit::draw();
	clipSmartDisable();
}

void UIDatePicker::updateButton() {
	if ( !mCalendarButton || mUpdatingButton )
		return;
	mUpdatingButton = true;
	const Float width = eeclamp( getSize().getHeight(), 0.f, getSize().getWidth() );
	mCalendarButton->setSize( width, getSize().getHeight() );
	mCalendarButton->setPosition( getSize().getWidth() - width, 0 );
	mReservedButtonWidth = width;
	setPaddingRight( mEditorRightPadding + width );
	mUpdatingButton = false;
}

void UIDatePicker::onSizeChange() {
	UIDateTimeEdit::onSizeChange();
	updateButton();
	if ( mCalendarVisible )
		UIPopUp::align( this, getCalendarPopup(), mPopUpToRoot );
}

void UIDatePicker::onPaddingChange() {
	if ( !mUpdatingButton && getPadding().Right != mEditorRightPadding + mReservedButtonWidth )
		mEditorRightPadding = getPadding().Right;
	UIDateTimeEdit::onPaddingChange();
	updateButton();
}

void UIDatePicker::onPositionChange() {
	UIDateTimeEdit::onPositionChange();
	if ( mCalendarVisible )
		UIPopUp::align( this, getCalendarPopup(), mPopUpToRoot );
}

void UIDatePicker::onVisibilityChange() {
	UIDateTimeEdit::onVisibilityChange();
	if ( !isVisible() )
		hideCalendar( false );
}

void UIDatePicker::onEnabledChange() {
	UIDateTimeEdit::onEnabledChange();
	if ( !isEnabled() )
		hideCalendar( false );
}

bool UIDatePicker::applyProperty( const StyleSheetProperty& attribute ) {
	if ( attribute.getPropertyDefinition() &&
		 attribute.getPropertyDefinition()->getPropertyId() == PropertyId::PopUpToRoot ) {
		mPopUpToRoot = attribute.asBool();
		if ( mCalendarVisible )
			UIPopUp::align( this, getCalendarPopup(), mPopUpToRoot );
		return true;
	}
	return UIDateTimeEdit::applyProperty( attribute );
}

std::string UIDatePicker::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& index ) const {
	if ( propertyDef && propertyDef->getPropertyId() == PropertyId::PopUpToRoot )
		return mPopUpToRoot ? "true" : "false";
	if ( propertyDef && propertyDef->getPropertyId() == PropertyId::PopUpOpen )
		return mCalendarVisible ? "true" : "false";
	return UIDateTimeEdit::getPropertyString( propertyDef, index );
}

std::vector<PropertyId> UIDatePicker::getPropertiesImplemented() const {
	auto properties = UIDateTimeEdit::getPropertiesImplemented();
	properties.push_back( PropertyId::PopUpToRoot );
	properties.push_back( PropertyId::PopUpOpen );
	return properties;
}

}} // namespace EE::UI
