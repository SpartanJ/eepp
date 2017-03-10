#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UICheckBox * UICheckBox::New() {
	return eeNew( UICheckBox, () );
}

UICheckBox::UICheckBox() :
	UITextView(),
	mActive( false ),
	mTextSeparation( 4 )
{
	mActiveButton 	= UIControlAnim::New();
	mActiveButton->setVisible( false );
	mActiveButton->setEnabled( true );
	mActiveButton->setParent( this );
	mActiveButton->setPosition( 0, 0 );
	mActiveButton->setSize( 16, 16 );

	mInactiveButton = UIControlAnim::New();
	mInactiveButton->setVisible( true );
	mInactiveButton->setEnabled( true );
	mInactiveButton->setParent( this );
	mInactiveButton->setPosition( 0, 0 );
	mInactiveButton->setSize( 16, 16 );

	onPaddingChange();

	applyDefaultTheme();
}


UICheckBox::~UICheckBox() {
}

Uint32 UICheckBox::getType() const {
	return UI_TYPE_CHECKBOX;
}

bool UICheckBox::isType( const Uint32& type ) const {
	return UICheckBox::getType() == type ? true : UITextView::isType( type );
}

void UICheckBox::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	setThemeControl( Theme, "checkbox" );

	mActiveButton->setThemeControl	( Theme, "checkbox_active" );
	mInactiveButton->setThemeControl( Theme, "checkbox_inactive" );

	onThemeLoaded();
}

void UICheckBox::onThemeLoaded() {
	UISkin * tSkin = mActiveButton->getSkin();

	if ( tSkin ) {
		mActiveButton->setSize( tSkin->getSize() );
		mActiveButton->centerVertical();
	}

	tSkin = mInactiveButton->getSkin();

	if ( NULL != tSkin ) {
		mInactiveButton->setSize( tSkin->getSize() );
		mInactiveButton->centerVertical();
	}

	mMinControlSize = mActiveButton->getSkinSize();

	onPaddingChange();
}

void UICheckBox::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( mSize.getWidth() == 0 ) {
			setInternalPixelsWidth( (int)mTextCache->getTextWidth() + mActiveButton->getRealSize().getWidth() + mTextSeparation );
		}

		if ( mSize.getHeight() == 0 ) {
			setInternalHeight( mActiveButton->getSize().getHeight() );
		}

		mActiveButton->centerVertical();
		mInactiveButton->centerVertical();
	}
}

void UICheckBox::onSizeChange() {
	UITextView::onSizeChange();

	mActiveButton->centerVertical();
	mInactiveButton->centerVertical();
}

Uint32 UICheckBox::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgClick: {
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				switchState();
			}

			if ( Msg->getSender() == mActiveButton || Msg->getSender() == mInactiveButton ) {
				sendMouseEvent( UIEvent::EventMouseClick, UIManager::instance()->getMousePos(), UIManager::instance()->getPressTrigger() );
			}

			return 1;
		}
	}

	return 0;
}

void UICheckBox::switchState() {
	setActive( !mActive );
}

void UICheckBox::setActive( const bool& active ) {
	if ( !active ) {
		mActiveButton->setVisible( false );
		mInactiveButton->setVisible( true );

		mActive = false;
	} else {
		mActiveButton->setVisible( true );
		mInactiveButton->setVisible( false );

		mActive = true;
	}

	onValueChange();
}

const bool& UICheckBox::isActive() const {
	return mActive;
}

void UICheckBox::onPaddingChange() {
	mActiveButton->setPosition( mPadding.Left, mActiveButton->getPosition().y );
	mInactiveButton->setPosition( mPadding.Left, mInactiveButton->getPosition().y );

	mRealPadding.Left = mActiveButton->getRealPosition().x + mActiveButton->getRealSize().getWidth() + PixelDensity::dpToPxI( mTextSeparation );
}

UIControlAnim * UICheckBox::getActiveButton() const {
	return mActiveButton;
}

UIControlAnim * UICheckBox::getInactiveButton() const {
	return mInactiveButton;
}

Int32 UICheckBox::getTextSeparation() const {
	return mTextSeparation;
}

void UICheckBox::setTextSeparation(const Int32 & textSeparation) {
	mTextSeparation = textSeparation;

	setPadding( getPadding() );
}

Uint32 UICheckBox::onKeyDown( const UIEventKey& Event ) {
	UITextView::onKeyDown( Event );

	if ( Event.getKeyCode() == KEY_SPACE ) {
		if ( Sys::getTicks() - mLastTick > 250 ) {
			mLastTick = Sys::getTicks();

			setActive( !mActive );
		}
	}

	return 1;
}

void UICheckBox::onAlphaChange() {
	UITextView::onAlphaChange();
	
	mActiveButton->setAlpha( mAlpha );
	mInactiveButton->setAlpha( mAlpha );
}

}}
