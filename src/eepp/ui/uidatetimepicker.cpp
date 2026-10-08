#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/ui/uicalendar.hpp>
#include <eepp/ui/uidatetimepicker.hpp>
#include <eepp/ui/uipopup.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitimepicker.hpp>

namespace EE { namespace UI {

namespace {

class PopupTimePicker;

class DateTimePopup : public UIWidget {
  public:
	DateTimePopup( UICalendar* calendar, PopupTimePicker* time );

	~DateTimePopup();

	void updateLayout();

  protected:
	void onAutoSize() override { updateLayout(); }

	void onSizeChange() override {
		UIWidget::onSizeChange();
		updateLayout();
	}

	void onPaddingChange() override {
		UIWidget::onPaddingChange();
		updateLayout();
	}

  private:
	UICalendar* mCalendar;
	PopupTimePicker* mTime;
	bool mUpdatingLayout{ false };
	EventConnection mCalendarSizeConnection;
	EventConnection mCalendarCloseConnection;
	EventConnection mTimeCloseConnection;
};

class PopupTimePicker : public UITimePicker {
  public:
	explicit PopupTimePicker( UIDateTimePicker* owner ) : mOwner( owner ) {
		setElementTag( "datetimepicker::time" );
		unsetFlags( UI_AUTO_SIZE );
		setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
		setClipType( ClipType::BorderBox );
		setTextAlign( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
	}

	void setPopup( DateTimePopup* popup ) { mPopup = popup; }

	void detachOwner() { mOwner = nullptr; }

	void commit() { commitPendingDigits( false ); }

	Sizef getPreferredSize() {
		return fitMinMaxSizeDp(
			PixelDensity::pxToDp(
				Sizef( getTextWidth() + mPaddingPx.getWidth(),
					   eemax( getTextHeight(), static_cast<Float>( mTextCache.getLineSpacing() ) ) +
						   mPaddingPx.getHeight() ) )
				.ceil() );
	}

	void updateArrows() {
		if ( mUpdatingArrows || !mTextCache.getFont() )
			return;
		mUpdatingArrows = true;
		for ( size_t i = 0; i < Sections.size(); ++i ) {
			const auto section = findSection( Sections[i] );
			if ( section && !mButtons[i][0] )
				createButtons( i );
			for ( size_t direction = 0; direction < 2; ++direction ) {
				auto* button = mButtons[i][direction];
				if ( !button )
					continue;
				button->setVisible( section.has_value() );
				button->setEnabled( section.has_value() && isEditingAllowed() );
				if ( !section )
					continue;
				const Float start = mTextCache.findCharacterPos( section->start ).x;
				const Float end = mTextCache.findCharacterPos( section->start + section->length ).x;
				const Float center = PixelDensity::pxToDp( mPaddingPx.Left + mRealAlignOffset.x +
														   ( start + end ) * 0.5f );
				const Float y =
					direction == 0
						? ( getPadding().Top - button->getSize().getHeight() ) * 0.5f
						: getSize().getHeight() - getPadding().Bottom +
							  ( getPadding().Bottom - button->getSize().getHeight() ) * 0.5f;
				button->setPosition( center - button->getSize().getWidth() * 0.5f, y );
			}
		}
		mUpdatingArrows = false;
	}

  protected:
	Uint32 onKeyDown( const KeyEvent& event ) override {
		if ( !mOwner )
			return UITimePicker::onKeyDown( event );
		if ( event.getKeyCode() == KEY_ESCAPE ) {
			resetPendingDigits();
			updateTextFromValue();
			mOwner->hideCalendar();
			return 1;
		}
		if ( event.getKeyCode() == KEY_RETURN || event.getKeyCode() == KEY_KP_ENTER ) {
			commit();
			mOwner->hideCalendar();
			return 1;
		}
		return UITimePicker::onKeyDown( event );
	}

	void draw() override {
		// Reserve top/bottom padding for the child arrows while clipping the inherited text.
		clipSmartEnable( mScreenPos.x + mPaddingPx.Left, mScreenPos.y + mPaddingPx.Top,
						 eemax( 0.f, mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right ),
						 eemax( 0.f, mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom ) );
		UITimePicker::draw();
		clipSmartDisable();
	}

	void onAutoSize() override {
		if ( mPopup )
			mPopup->updateLayout();
		else
			UITimePicker::onAutoSize();
		updateArrows();
	}

	void onSizeChange() override {
		UITimePicker::onSizeChange();
		if ( mPopup )
			mPopup->updateLayout();
		updateArrows();
	}

	void onFontChanged() override {
		UITimePicker::onFontChanged();
		onAutoSize();
	}

	void onFontStyleChanged() override {
		UITimePicker::onFontStyleChanged();
		onAutoSize();
	}

	void onPaddingChange() override {
		UITimePicker::onPaddingChange();
		onAutoSize();
	}

	void alignFix() override {
		UITimePicker::alignFix();
		// The input retains its scrolling offset. Honor CSS alignment when the whole time fits.
		if ( getTextWidth() <= mSize.getWidth() - mPaddingPx.getWidth() )
			UITextView::alignFix();
		updateArrows();
	}

	void onDocumentSelectionChange( const TextRange& range ) override {
		UITimePicker::onDocumentSelectionChange( range );
		updateArrows();
	}

  private:
	void createButtons( size_t index ) {
		const auto section = Sections[index];
		for ( size_t direction = 0; direction < 2; ++direction ) {
			auto* button = UIPushButton::NewWithTag( direction == 0 ? "datetimepicker::time-up"
																	: "datetimepicker::time-down" );
			button->addClass( SectionNames[index] );
			button->unsetFlags( UI_AUTO_SIZE );
			button->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
			button->setSize( 24, 22 );
			button->setParent( this );
			const Int32 delta = direction == 0 ? 1 : -1;
			mButtonConnections[index][direction] =
				button->connect( Event::MouseClick, [this, section, delta]( const Event* event ) {
					if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) || !mOwner ||
						 !mOwner->isEnabled() || !mOwner->isEditingAllowed() ||
						 !mOwner->isCalendarVisible() )
						return;
					setActiveSection( section );
					stepActiveSection( delta );
					setFocus();
				} );
			mButtons[index][direction] = button;
		}
	}

	static constexpr std::array<DateTimeSection, 5> Sections{
		DateTimeSection::Hour, DateTimeSection::Minute, DateTimeSection::Second,
		DateTimeSection::Millisecond, DateTimeSection::AmPm };
	static constexpr std::array<const char*, 5> SectionNames{ "hour", "minute", "second",
															  "millisecond", "am-pm" };
	UIDateTimePicker* mOwner;
	DateTimePopup* mPopup{ nullptr };
	std::array<std::array<UIPushButton*, 2>, 5> mButtons{};
	bool mUpdatingArrows{ false };
	std::array<std::array<EventConnection, 2>, 5> mButtonConnections;
};

DateTimePopup::DateTimePopup( UICalendar* calendar, PopupTimePicker* time ) :
	UIWidget( "datetimepicker::popup" ), mCalendar( calendar ), mTime( time ) {
	setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
	mCalendar->setParent( this );
	mTime->setParent( this );
	mCalendarSizeConnection =
		mCalendar->connect( Event::OnSizeChange, [this]( const Event* ) { updateLayout(); } );
	mCalendarCloseConnection =
		mCalendar->connect( Event::OnClose, [this]( const Event* ) { mCalendar = nullptr; } );
	mTimeCloseConnection =
		mTime->connect( Event::OnClose, [this]( const Event* ) { mTime = nullptr; } );
	mTime->setPopup( this );
	updateLayout();
}

DateTimePopup::~DateTimePopup() {
	// Children are deleted by Node after this derived object has finished destruction.
	mCalendarSizeConnection.disconnect();
	mCalendarCloseConnection.disconnect();
	mTimeCloseConnection.disconnect();
	if ( mTime ) {
		mTime->setPopup( nullptr );
	}
}

void DateTimePopup::updateLayout() {
	if ( mUpdatingLayout || !mCalendar || !mTime || isClosing() )
		return;
	mUpdatingLayout = true;
	const Sizef timeSize( mTime->getPreferredSize() );
	const Float width = eemax( mCalendar->getSize().getWidth(), timeSize.getWidth() );
	const Float height = mCalendar->getSize().getHeight() + timeSize.getHeight();
	const Sizef size = fitMinMaxSizeDp(
		{ getLayoutWidthPolicy() == SizePolicy::WrapContent ? width + getPadding().getWidth()
															: getSize().getWidth(),
		  getLayoutHeightPolicy() == SizePolicy::WrapContent ? height + getPadding().getHeight()
															 : getSize().getHeight() } );
	setSize( size );
	const Float contentWidth = eemax( 0.f, size.getWidth() - getPadding().getWidth() );
	mCalendar->setPosition( getPadding().Left +
								( contentWidth - mCalendar->getSize().getWidth() ) * 0.5f,
							getPadding().Top );
	mTime->setSize( contentWidth, timeSize.getHeight() );
	mTime->setPosition( getPadding().Left, getPadding().Top + mCalendar->getSize().getHeight() );
	mTime->updateArrows();
	mUpdatingLayout = false;
}

} // namespace

UIDateTimePicker* UIDateTimePicker::New() {
	return eeNew( UIDateTimePicker, () );
}

UIDateTimePicker::UIDateTimePicker() :
	UIDatePicker( DateTimeEditMode::DateTime, "datetimepicker" ) {
	mConnections[0] =
		connect( Event::OnValueChange, [this]( const Event* ) { synchronizeTimePopup(); } );
	mConnections[1] =
		connect( Event::OnTextChanged, [this]( const Event* ) { synchronizeTimePopup(); } );
}

UIDateTimePicker::~UIDateTimePicker() {
	// The popup may be parented to a window/root and therefore needs explicit owner cleanup.
	auto* popup = mPopup;
	auto* time = static_cast<PopupTimePicker*>( mPopupTimePicker );
	const bool deferred =
		popup && ( popup->isClosing() || ( mCalendar && mCalendar->isClosing() ) ||
				   ( time && time->isClosing() ) );
	mCalendarVisible = false;
	mConnections = {};
	mCalendarConnections = {};
	mTimeConnections = {};
	mPopupConnections = {};
	if ( time ) {
		time->detachOwner();
	}
	mPopup = nullptr;
	mPopupTimePicker = nullptr;
	if ( popup ) {
		mCalendar = nullptr;
		// Deleting a queued child or popup here would leave its pointer in the scene's close
		// list. Let the scene finish that deletion, with all owner callbacks disconnected.
		if ( deferred ) {
			popup->setEnabled( false );
			popup->setVisible( false );
			if ( !popup->isClosing() )
				popup->close();
		} else {
			eeDelete( popup );
		}
	}
}

Uint32 UIDateTimePicker::getType() const {
	return UI_TYPE_DATETIMEPICKER;
}

bool UIDateTimePicker::isType( const Uint32& type ) const {
	return type == UI_TYPE_DATETIMEPICKER || UIDatePicker::isType( type );
}

UIWidget* UIDateTimePicker::getCalendarPopup() const {
	return mPopup;
}

UITimePicker* UIDateTimePicker::getPopupTimePicker() const {
	return mPopupTimePicker;
}

void UIDateTimePicker::prepareCalendarPopup() {
	if ( mPopup &&
		 ( !mPopupTimePicker || mCalendar->getParent() != mPopup || mPopup->isClosing() ) ) {
		// A closed child calendar/time editor must not leave a stale composite on the next open.
		mCalendar->setParent( this );
		auto* popup = mPopup;
		mPopup = nullptr;
		mPopupTimePicker = nullptr;
		eeDelete( popup );
	}
	if ( !mPopup ) {
		auto* time = eeNew( PopupTimePicker, ( this ) );
		mPopupTimePicker = time;
		mPopup = eeNew( DateTimePopup, ( mCalendar, time ) );
		mPopup->setEnabled( false );
		mPopup->setVisible( false );
		mPopup->setParent( this );
		mPopupConnections[0] =
			mPopup->connect( Event::OnClose, [this, popup = mPopup]( const Event* ) {
				if ( mPopup != popup )
					return;
				mCalendar = nullptr;
				mPopup = nullptr;
				mPopupTimePicker = nullptr;
				mCalendarVisible = false;
				mSceneSizeConnection.disconnect();
			} );
		mPopupConnections[1] = mPopup->connect( Event::OnSizeChange, [this]( const Event* ) {
			if ( mCalendarVisible )
				UIPopUp::align( this, mPopup, mPopUpToRoot );
		} );
		mPopupConnections[2] = mPopup->connect( Event::OnFocusWithinLoss,
												[this]( const Event* ) { closeOnFocusLoss(); } );
		mPopupConnections[3] = mPopup->connect( Event::KeyDown, [this]( const Event* event ) {
			if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
				hideCalendar();
		} );
		mTimeConnections[0] =
			time->connect( Event::OnValueChange, [this]( const Event* ) { selectPopupTime(); } );
		mTimeConnections[1] = time->connect( Event::OnClose, [this, time]( const Event* ) {
			if ( mPopupTimePicker == time ) {
				mPopupTimePicker = nullptr;
				hideCalendar( false );
			}
		} );
	}
	mCalendar->setEnabled( true );
	mCalendar->setVisible( true );
	synchronizeTimePopup();
}

void UIDateTimePicker::synchronizeTimePopup() {
	if ( !mPopup || !mPopupTimePicker || !mCalendar || mSynchronizingPopup )
		return;
	mSynchronizingPopup = true;
	auto* time = static_cast<PopupTimePicker*>( mPopupTimePicker );
	// Only the time-relevant locale fields affect this editor. Avoid copying/recompiling
	// the complete locale on each value change.
	if ( time->getLocale().hourCycle != getLocale().hourCycle ||
		 time->getLocale().am != getLocale().am || time->getLocale().pm != getLocale().pm )
		time->setLocale( getLocale() );
	time->setHourCycle( getHourCycle() );
	time->setShowSeconds( getShowSeconds() );
	time->setShowMilliseconds( getShowMilliseconds() );
	time->setTimeFormat( getTimeFormat() );
	time->setHourStep( getHourStep() );
	time->setMinuteStep( getMinuteStep() );
	time->setSecondStep( getSecondStep() );
	time->setPendingDigitTimeout( getPendingDigitTimeout() );
	time->setAllowEmpty( false );
	time->setAllowEditing( isEditingAllowed() && isEnabled() );
	const CalendarDate date = getDateTime() ? getDateTime()->date : mCalendar->getFocusedDate();
	const std::optional<TimeOfDay> minimum =
		getMinDateTime() && getMinDateTime()->date == date
			? std::optional<TimeOfDay>( getMinDateTime()->time )
			: std::nullopt;
	const std::optional<TimeOfDay> maximum =
		getMaxDateTime() && getMaxDateTime()->date == date
			? std::optional<TimeOfDay>( getMaxDateTime()->time )
			: std::nullopt;
	if ( time->getMinTime() != minimum || time->getMaxTime() != maximum ) {
		time->setMinTime( std::nullopt );
		time->setMaxTime( std::nullopt );
		time->setMinTime( minimum );
		time->setMaxTime( maximum );
	}
	TimeOfDay value = getDateTime() ? getDateTime()->time : TimeOfDay{};
	if ( minimum && value < *minimum )
		value = *minimum;
	if ( maximum && value > *maximum )
		value = *maximum;
	if ( time->getTime() != value )
		time->setTime( value );
	if ( getDateTime() && mCalendar->getSelectedDate() != getDateTime()->date )
		mCalendar->setSelectedDate( getDateTime()->date );
	else if ( !getDateTime() && mCalendar->getSelectedDate() )
		mCalendar->clearSelection();
	time->updateArrows();
	static_cast<DateTimePopup*>( mPopup )->updateLayout();
	mSynchronizingPopup = false;
}

void UIDateTimePicker::onConstraintsChange() {
	UIDatePicker::onConstraintsChange();
	synchronizeTimePopup();
}

void UIDateTimePicker::onCalendarFocusedDateChange() {
	synchronizeTimePopup();
}

void UIDateTimePicker::onConfigurationChange() {
	UIDatePicker::onConfigurationChange();
	synchronizeTimePopup();
}

void UIDateTimePicker::selectCalendarDate() {
	if ( !mCalendar || !mCalendar->getSelectedDate() || !isEnabled() || !isEditingAllowed() )
		return;
	setDateTime( LocalDateTime{ *mCalendar->getSelectedDate(),
								getDateTime() ? getDateTime()->time : TimeOfDay{} } );
	synchronizeTimePopup();
}

void UIDateTimePicker::selectPopupTime() {
	if ( mSynchronizingPopup || !mCalendar || !mPopupTimePicker )
		return;
	if ( isCalendarVisible() && isEnabled() && isEditingAllowed() && mPopupTimePicker->getTime() ) {
		const CalendarDate date = getDateTime() ? getDateTime()->date : mCalendar->getFocusedDate();
		setDateTime( LocalDateTime{ date, *mPopupTimePicker->getTime() } );
	}
	synchronizeTimePopup();
}

UIDateTimePicker* UIDateTimePicker::hideCalendar( bool restoreFocus ) {
	if ( mPopupTimePicker && !mSynchronizingPopup )
		static_cast<PopupTimePicker*>( mPopupTimePicker )->commit();
	UIDatePicker::hideCalendar( restoreFocus );
	return this;
}

}} // namespace EE::UI
