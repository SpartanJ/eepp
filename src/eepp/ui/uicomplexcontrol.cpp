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

	tooltipText( Params.TooltipText );
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
		mDistToBorder	= Recti( mPos.x, mPos.y, mParentCtrl->size().x - ( mPos.x + mSize.x ), mParentCtrl->size().y - ( mPos.y + mSize.y ) );
	}
}

void UIComplexControl::update() {
	if ( mVisible && NULL != mTooltip && mTooltip->text().size() ) {
		if ( isMouseOverMeOrChilds() ) {
			Vector2i Pos = UIManager::instance()->getMousePos();
			Pos.x += UIThemeManager::instance()->cursorSize().x;
			Pos.y += UIThemeManager::instance()->cursorSize().y;

			if ( Pos.x + mTooltip->size().width() > UIManager::instance()->mainControl()->size().width() ) {
				Pos.x = UIManager::instance()->getMousePos().x - mTooltip->size().width();
			}

			if ( Pos.y + mTooltip->size().height() > UIManager::instance()->mainControl()->size().height() ) {
				Pos.y = UIManager::instance()->getMousePos().y - mTooltip->size().height();
			}

			if ( Time::Zero == UIThemeManager::instance()->tooltipTimeToShow() ) {
				if ( !mTooltip->visible() || UIThemeManager::instance()->tooltipFollowMouse() )
					mTooltip->position( Pos );

				mTooltip->show();
			} else {
				if ( -1.f != mTooltip->tooltipTime().asMilliseconds() ) {
					mTooltip->tooltipTimeAdd( UIManager::instance()->elapsed() );
				}

				if ( mTooltip->tooltipTime() >= UIThemeManager::instance()->tooltipTimeToShow() ) {
					if ( mTooltip->tooltipTime().asMilliseconds() != -1.f ) {
						mTooltip->position( Pos );

						mTooltip->show();

						mTooltip->tooltipTime( Milliseconds( -1.f ) );
					}
				}
			}

			if ( UIThemeManager::instance()->tooltipFollowMouse() ) {
				mTooltip->position( Pos );
			}
		} else {
			mTooltip->tooltipTime( Milliseconds( 0.f ) );

			if ( mTooltip->visible() )
				mTooltip->hide();
		}
	}

	UIControlAnim::update();
}

void UIComplexControl::createTooltip() {
	if ( NULL != mTooltip )
		return;

	UITheme * tTheme = UIThemeManager::instance()->defaultTheme();

	if ( NULL != tTheme ) {
		mTooltip = tTheme->createTooltip( this, UIManager::instance()->mainControl() );
		mTooltip->visible( false );
		mTooltip->enabled( false );
	} else {
		UITooltip::CreateParams Params;
		Params.setParent( UIManager::instance()->mainControl() );
		Params.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_PADDING | UI_AUTO_SIZE;
		mTooltip = eeNew( UITooltip, ( Params, this ) );
	}
}

void UIComplexControl::tooltipText( const String& Text ) {
	if ( NULL == mTooltip ) {	// If the tooltip wasn't created it will avoid to create a new one if the string is ""
		if ( Text.size() ) {
			createTooltip();

			mTooltip->text( Text );
		}
	} else { // but if it's created, i will allow it
		mTooltip->text( Text );
	}
}

String UIComplexControl::tooltipText() {
	if ( NULL != mTooltip )
		return mTooltip->text();

	return String();
}

void UIComplexControl::tooltipRemove() {
	mTooltip = NULL;
}

void UIComplexControl::size( const Sizei &Size ) {
	Sizei s( Size );

	if ( s.x < mMinControlSize.x )
		s.x = mMinControlSize.x;

	if ( s.y < mMinControlSize.y )
		s.y = mMinControlSize.y;

	UIControlAnim::size( s );
}

void UIComplexControl::size( const Int32& Width, const Int32& Height ) {
	UIControlAnim::size( Width, Height );
}

const Sizei& UIComplexControl::size() {
	return UIControlAnim::size();
}

void UIComplexControl::onParentSizeChange( const Vector2i& SizeChange ) {
	Sizei newSize( mSize );

	if ( mFlags & UI_ANCHOR_LEFT ) {
		// Nothing ?
	} else {
		position( mPos.x += SizeChange.x, mPos.y );
	}

	if ( mFlags & UI_ANCHOR_RIGHT ) {
		if ( NULL != mParentCtrl ) {
			newSize.x = mParentCtrl->size().width() - mPos.x - mDistToBorder.Right;

			if ( newSize.x < mMinControlSize.width() )
				newSize.x = mMinControlSize.width();
		}
	}

	if ( mFlags & UI_ANCHOR_TOP ) {
		// Nothing ?
	} else {
		position( mPos.x, mPos.y += SizeChange.y );
	}

	if ( mFlags & UI_ANCHOR_BOTTOM ) {
		if ( NULL != mParentCtrl ) {
			newSize.y = mParentCtrl->size().y - mPos.y - mDistToBorder.Bottom;

			if ( newSize.y < mMinControlSize.height() )
				newSize.y = mMinControlSize.height();
		}
	}

	if ( newSize != mSize )
		size( newSize );

	UIControlAnim::onParentSizeChange( SizeChange );
}

}}
