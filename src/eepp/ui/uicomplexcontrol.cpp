#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIComplexControl::UIComplexControl( const UIComplexControl::CreateParams& Params ) :
	UIControlAnim( Params ),
	mTooltip( NULL ),
	mMinControlSize( Params.MinControlSize )
{
	mControlFlags |= UI_CTRL_FLAG_COMPLEX;

	updateAnchorsDistances();

	setTooltipText( Params.TooltipText );
}

UIComplexControl::~UIComplexControl() {
	eeSAFE_DELETE( mTooltip );
}

Uint32 UIComplexControl::getType() const {
	return UI_TYPE_CONTROL_COMPLEX;
}

bool UIComplexControl::isType( const Uint32& type ) const {
	return UIComplexControl::getType() == type ? true : UIControlAnim::isType( type );
}

void UIComplexControl::updateAnchorsDistances() {
	if ( NULL != mParentCtrl ) {
		mDistToBorder	= Recti( mPos.x, mPos.y, mParentCtrl->getSize().x - ( mPos.x + mSize.x ), mParentCtrl->getSize().y - ( mPos.y + mSize.y ) );
	}
}

void UIComplexControl::update() {
	if ( mVisible && NULL != mTooltip && mTooltip->getText().size() ) {
		if ( isMouseOverMeOrChilds() ) {
			Vector2i Pos = UIManager::instance()->getMousePos();
			Pos.x += UIThemeManager::instance()->getCursorSize().x;
			Pos.y += UIThemeManager::instance()->getCursorSize().y;

			if ( Pos.x + mTooltip->getSize().getWidth() > UIManager::instance()->getMainControl()->getSize().getWidth() ) {
				Pos.x = UIManager::instance()->getMousePos().x - mTooltip->getSize().getWidth();
			}

			if ( Pos.y + mTooltip->getSize().getHeight() > UIManager::instance()->getMainControl()->getSize().getHeight() ) {
				Pos.y = UIManager::instance()->getMousePos().y - mTooltip->getSize().getHeight();
			}

			if ( Time::Zero == UIThemeManager::instance()->getTooltipTimeToShow() ) {
				if ( !mTooltip->isVisible() || UIThemeManager::instance()->getTooltipFollowMouse() )
					mTooltip->setPosition( Pos );

				mTooltip->show();
			} else {
				if ( -1.f != mTooltip->getTooltipTime().asMilliseconds() ) {
					mTooltip->addTooltipTime( UIManager::instance()->getElapsed() );
				}

				if ( mTooltip->getTooltipTime() >= UIThemeManager::instance()->getTooltipTimeToShow() ) {
					if ( mTooltip->getTooltipTime().asMilliseconds() != -1.f ) {
						mTooltip->setPosition( Pos );

						mTooltip->show();

						mTooltip->setTooltipTime( Milliseconds( -1.f ) );
					}
				}
			}

			if ( UIThemeManager::instance()->getTooltipFollowMouse() ) {
				mTooltip->setPosition( Pos );
			}
		} else {
			mTooltip->setTooltipTime( Milliseconds( 0.f ) );

			if ( mTooltip->isVisible() )
				mTooltip->hide();
		}
	}

	UIControlAnim::update();
}

void UIComplexControl::createTooltip() {
	if ( NULL != mTooltip )
		return;

	UITheme * tTheme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != tTheme ) {
		mTooltip = tTheme->createTooltip( this, UIManager::instance()->getMainControl() );
		mTooltip->setVisible( false );
		mTooltip->setEnabled( false );
	} else {
		UITooltip::CreateParams Params;
		Params.setParent( UIManager::instance()->getMainControl() );
		Params.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_PADDING | UI_AUTO_SIZE;
		mTooltip = eeNew( UITooltip, ( Params, this ) );
	}
}

void UIComplexControl::setTooltipText( const String& Text ) {
	if ( NULL == mTooltip ) {	// If the tooltip wasn't created it will avoid to create a new one if the string is ""
		if ( Text.size() ) {
			createTooltip();

			mTooltip->setText( Text );
		}
	} else { // but if it's created, i will allow it
		mTooltip->setText( Text );
	}
}

String UIComplexControl::getTooltipText() {
	if ( NULL != mTooltip )
		return mTooltip->getText();

	return String();
}

void UIComplexControl::tooltipRemove() {
	mTooltip = NULL;
}

void UIComplexControl::setSize( const Sizei& size ) {
	Sizei s( size );

	if ( s.x < mMinControlSize.x )
		s.x = mMinControlSize.x;

	if ( s.y < mMinControlSize.y )
		s.y = mMinControlSize.y;

	UIControlAnim::setSize( s );
}

void UIComplexControl::setSize( const Int32& Width, const Int32& Height ) {
	UIControlAnim::setSize( Width, Height );
}

const Sizei& UIComplexControl::getSize() {
	return UIControlAnim::getSize();
}

void UIComplexControl::onParentSizeChange( const Vector2i& SizeChange ) {
	Sizei newSize( mSize );

	if ( mFlags & UI_ANCHOR_LEFT ) {
		// Nothing ?
	} else {
		setPosition( mPos.x += SizeChange.x, mPos.y );
	}

	if ( mFlags & UI_ANCHOR_RIGHT ) {
		if ( NULL != mParentCtrl ) {
			newSize.x = mParentCtrl->getSize().getWidth() - mPos.x - mDistToBorder.Right;

			if ( newSize.x < mMinControlSize.getWidth() )
				newSize.x = mMinControlSize.getWidth();
		}
	}

	if ( mFlags & UI_ANCHOR_TOP ) {
		// Nothing ?
	} else {
		setPosition( mPos.x, mPos.y += SizeChange.y );
	}

	if ( mFlags & UI_ANCHOR_BOTTOM ) {
		if ( NULL != mParentCtrl ) {
			newSize.y = mParentCtrl->getSize().y - mPos.y - mDistToBorder.Bottom;

			if ( newSize.y < mMinControlSize.getHeight() )
				newSize.y = mMinControlSize.getHeight();
		}
	}

	if ( newSize != mSize )
		setSize( newSize );

	UIControlAnim::onParentSizeChange( SizeChange );
}

}}
