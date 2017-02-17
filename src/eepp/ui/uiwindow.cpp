#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/primitives.hpp>

namespace EE { namespace UI {

UIWindow::UIWindow( const UIWindow::CreateParams& Params ) :
	UIComplexControl( Params ),
	mWinFlags( Params.WinFlags ),
	mWindowDecoration( NULL ),
	mButtonClose( NULL ),
	mButtonMinimize( NULL ),
	mButtonMaximize( NULL ),
	mTitle( NULL ),
	mModalCtrl( NULL ),
	mDecoSize( Params.DecorationSize ),
	mBorderSize( Params.BorderSize ),
	mMinWindowSize( Params.MinWindowSize ),
	mButtonsPositionFixer( Params.ButtonsPositionFixer ),
	mButtonsSeparation( Params.ButtonsSeparation ),
	mMinCornerDistance( Params.MinCornerDistance ),
	mResizeType( RESIZE_NONE ),
	mTitleFontColor( Params.TitleFontColor ),
	mBaseAlpha( Params.BaseAlpha ),
	mDecoAutoSize( Params.DecorationAutoSize ),
	mBorderAutoSize( Params.BorderAutoSize )
{
	UIManager::instance()->windowAdd( this );

	UIComplexControl::CreateParams tcParams;
	tcParams.setParent( this );
	tcParams.Flags |= UI_REPORT_SIZE_CHANGE_TO_CHILDS;

	mContainer		= eeNew( UIComplexControl, ( tcParams ) );
	mContainer->enabled( true );
	mContainer->visible( true );
	mContainer->size( mSize );
	mContainer->addEventListener( UIEvent::EventOnPosChange, cb::Make1( this, &UIWindow::containerPosChange ) );

	if ( !( mWinFlags & UI_WIN_NO_BORDER ) ) {
		UIControlAnim::CreateParams tParams;
		tParams.setParent( this );

		mWindowDecoration = eeNew( UIControlAnim, ( tParams ) );
		mWindowDecoration->visible( true );
		mWindowDecoration->enabled( false );

		mBorderLeft		= eeNew( UIControlAnim, ( tParams ) );
		mBorderLeft->enabled( true );
		mBorderLeft->visible( true );

		mBorderRight	= eeNew( UIControlAnim, ( tParams ) );
		mBorderRight->enabled( true );
		mBorderRight->visible( true );

		mBorderBottom	= eeNew( UIControlAnim, ( tParams ) );
		mBorderBottom->enabled( true );
		mBorderBottom->visible( true );

		if ( mWinFlags & UI_WIN_DRAGABLE_CONTAINER )
			mContainer->dragEnable( true );

		UIComplexControl::CreateParams ButtonParams;
		ButtonParams.setParent( this );

		if ( mWinFlags & UI_WIN_CLOSE_BUTTON ) {
			mButtonClose = eeNew( UIComplexControl, ( ButtonParams ) );
			mButtonClose->visible( true );
			mButtonClose->enabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonClose->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIWindow::buttonCloseClick ) );
			}
		}

		if ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( mWinFlags & UI_WIN_MAXIMIZE_BUTTON ) ) {
			mButtonMaximize = eeNew( UIComplexControl, ( ButtonParams ) );
			mButtonMaximize->visible( true );
			mButtonMaximize->enabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonMaximize->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIWindow::buttonMaximizeClick ) );
			}
		}

		if ( mWinFlags & UI_WIN_MINIMIZE_BUTTON ) {
			mButtonMinimize = eeNew( UIComplexControl, ( ButtonParams ) );
			mButtonMinimize->visible( true );
			mButtonMinimize->enabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonMinimize->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIWindow::buttonMinimizeClick ) );
			}
		}

		dragEnable( true );
	}

	if ( isModal() ) {
		createModalControl();
	}

	alpha( mBaseAlpha );

	applyDefaultTheme();
}

UIWindow::~UIWindow() {
	UIManager::instance()->windowRemove( this );

	UIManager::instance()->setFocusLastWindow( this );

	sendCommonEvent( UIEvent::EventOnWindowClose );
}

void UIWindow::createModalControl() {
	UIControl * Ctrl = UIManager::instance()->mainControl();

	if ( NULL == mModalCtrl ) {
		mModalCtrl = eeNew( UIControlAnim, ( UIControlAnim::CreateParams( Ctrl , Vector2i(0,0), Ctrl->size(), UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) );
	} else {
		mModalCtrl->position( 0, 0 );
		mModalCtrl->size( Ctrl->size() );
	}

	disableByModal();
}

void UIWindow::enableByModal() {
	if ( isModal() ) {
		UIControl * CtrlChild = UIManager::instance()->mainControl()->childGetFirst();

		while ( NULL != CtrlChild )
		{
			if ( CtrlChild != mModalCtrl &&
				 CtrlChild != this &&
				 CtrlChild->controlFlags() & UI_CTRL_FLAG_DISABLED_BY_MODAL_WINDOW )
			{
				CtrlChild->enabled( true );
				CtrlChild->writeCtrlFlag( UI_CTRL_FLAG_DISABLED_BY_MODAL_WINDOW, 0 );
			}

			CtrlChild = CtrlChild->nextGet();
		}
	}
}

void UIWindow::disableByModal() {
	if ( isModal() ) {
		UIControl * CtrlChild = UIManager::instance()->mainControl()->childGetFirst();

		while ( NULL != CtrlChild )
		{
			if ( CtrlChild != mModalCtrl &&
				 CtrlChild != this &&
				 CtrlChild->enabled() )
			{
				CtrlChild->enabled( false );
				CtrlChild->writeCtrlFlag( UI_CTRL_FLAG_DISABLED_BY_MODAL_WINDOW, 1 );
			}

			CtrlChild = CtrlChild->nextGet();
		}
	}
}

Uint32 UIWindow::getType() const {
	return UI_TYPE_WINDOW;
}

bool UIWindow::isType( const Uint32& type ) const {
	return UIWindow::getType() == type ? true : UIComplexControl::isType( type );
}

void UIWindow::containerPosChange( const UIEvent * Event ) {
	Vector2i PosDiff = mContainer->position() - Vector2i( mBorderLeft->size().getWidth(), mWindowDecoration->size().getHeight() );

	if ( PosDiff.x != 0 || PosDiff.y != 0 ) {
		mContainer->position( mBorderLeft->size().getWidth(), mWindowDecoration->size().getHeight() );

		position( mPos + PosDiff );
	}
}

void UIWindow::buttonCloseClick( const UIEvent * Event ) {
	CloseWindow();

	sendCommonEvent( UIEvent::EventOnWindowCloseClick );
}

void UIWindow::CloseWindow() {
	if ( NULL != mButtonClose )
		mButtonClose->enabled( false );

	if ( NULL != mButtonMaximize )
		mButtonMaximize->enabled( false );

	if ( NULL != mButtonMinimize )
		mButtonMinimize->enabled( false );

	if ( NULL != mModalCtrl ) {
		mModalCtrl->close();
		mModalCtrl = NULL;
	}

	if ( Time::Zero != UIThemeManager::instance()->controlsFadeOutTime() )
		closeFadeOut( UIThemeManager::instance()->controlsFadeOutTime() );
	else
		close();
}

void UIWindow::close() {
	UIComplexControl::close();

	enableByModal();
}

void UIWindow::buttonMaximizeClick( const UIEvent * Event ) {
	maximize();

	sendCommonEvent( UIEvent::EventOnWindowMaximizeClick );
}

void UIWindow::buttonMinimizeClick( const UIEvent * Event ) {
	Hide();

	sendCommonEvent( UIEvent::EventOnWindowMinimizeClick );
}

void UIWindow::setTheme( UITheme *Theme ) {
	UIComplexControl::setTheme( Theme );

	mContainer->setThemeControl			( Theme, "winback"			);

	if ( !( mWinFlags & UI_WIN_NO_BORDER ) ) {
		mWindowDecoration->setThemeControl	( Theme, "windeco"			);
		mBorderLeft->setThemeControl		( Theme, "winborderleft"	);
		mBorderRight->setThemeControl		( Theme, "winborderright"	);
		mBorderBottom->setThemeControl		( Theme, "winborderbottom"	);

		if ( NULL != mButtonClose ) {
			mButtonClose->setThemeControl( Theme, "winclose" );
			mButtonClose->size( mButtonClose->getSkinSize() );
		}

		if ( NULL != mButtonMaximize ) {
			mButtonMaximize->setThemeControl( Theme, "winmax" );
			mButtonMaximize->size( mButtonMaximize->getSkinSize() );
		}

		if ( NULL != mButtonMinimize ) {
			mButtonMinimize->setThemeControl( Theme, "winmin" );
			mButtonMinimize->size( mButtonMinimize->getSkinSize() );
		}

		fixChildsSize();
		getMinWinSize();
	}
}

void UIWindow::getMinWinSize() {
	if ( NULL == mWindowDecoration || ( mMinWindowSize.x != 0 && mMinWindowSize.y != 0 ) )
		return;

	Sizei tSize;

	tSize.x = mBorderLeft->size().getWidth() + mBorderRight->size().getWidth() - mButtonsPositionFixer.x;
	tSize.y = mWindowDecoration->size().getHeight() + mBorderBottom->size().getHeight();

	if ( NULL != mButtonClose )
		tSize.x += mButtonClose->size().getWidth();

	if ( NULL != mButtonMaximize )
		tSize.x += mButtonMaximize->size().getWidth();

	if ( NULL != mButtonMinimize )
		tSize.x += mButtonMinimize->size().getWidth();

	if ( mMinWindowSize.x < tSize.x )
		mMinWindowSize.x = tSize.x;

	if ( mMinWindowSize.y < tSize.y )
		mMinWindowSize.y = tSize.y;
}

void UIWindow::onSizeChange() {
	if ( mSize.x < mMinWindowSize.x || mSize.y < mMinWindowSize.y ) {
		if ( mSize.x < mMinWindowSize.x && mSize.y < mMinWindowSize.y ) {
			size( mMinWindowSize );
		} else if ( mSize.x < mMinWindowSize.x ) {
			size( Sizei( mMinWindowSize.x, mSize.y ) );
		} else {
			size( Sizei( mSize.x, mMinWindowSize.y ) );
		}
	} else {
		fixChildsSize();

		UIComplexControl::onSizeChange();
	}
}

void UIWindow::size( const Sizei& Size ) {
	if ( NULL != mWindowDecoration ) {
		Sizei size = Size;

		size.x += mBorderLeft->size().getWidth() + mBorderRight->size().getWidth();
		size.y += mWindowDecoration->size().getHeight() + mBorderBottom->size().getHeight();

		UIComplexControl::size( size );
	} else {
		UIComplexControl::size( Size );
	}
}

void UIWindow::size( const Int32& Width, const Int32& Height ) {
	size( Sizei( Width, Height ) );
}

const Sizei& UIWindow::size() {
	return UIComplexControl::size();
}

void UIWindow::fixChildsSize() {
	if ( NULL == mWindowDecoration ) {
		mContainer->size( mSize.getWidth(), mSize.getHeight() );
		return;
	}

	if ( mDecoAutoSize ) {
		mDecoSize = Sizei( mSize.getWidth(), mWindowDecoration->getSkinSize().getHeight() );
	}

	mWindowDecoration->size( mDecoSize );

	if ( mBorderAutoSize ) {
		mBorderBottom->size( mSize.getWidth(), mBorderBottom->getSkinSize().getHeight() );
	} else {
		mBorderBottom->size( mSize.getWidth(), mBorderSize.getHeight() );
	}

	Uint32 BorderHeight = mSize.getHeight() - mDecoSize.getHeight() - mBorderBottom->size().getHeight();

	if ( mBorderAutoSize ) {
		mBorderLeft->size( mBorderLeft->getSkinSize().getWidth()	, BorderHeight );
		mBorderRight->size( mBorderRight->getSkinSize().getWidth(), BorderHeight );
	} else {
		mBorderLeft->size( mBorderSize.getWidth(), BorderHeight );
		mBorderRight->size( mBorderSize.getWidth(), BorderHeight );
	}

	mBorderLeft->position( 0, mWindowDecoration->size().getHeight() );
	mBorderRight->position( mSize.getWidth() - mBorderRight->size().getWidth(), mWindowDecoration->size().getHeight() );
	mBorderBottom->position( 0, mSize.getHeight() - mBorderBottom->size().getHeight() );

	mContainer->position( mBorderLeft->size().getWidth(), mWindowDecoration->size().getHeight() );
	mContainer->size( mSize.getWidth() - mBorderLeft->size().getWidth() - mBorderRight->size().getWidth(), mSize.getHeight() - mWindowDecoration->size().getHeight() - mBorderBottom->size().getHeight() );

	Uint32 yPos;

	if ( NULL != mButtonClose ) {
		yPos = mWindowDecoration->size().getHeight() / 2 - mButtonClose->size().getHeight() / 2 + mButtonsPositionFixer.y;

		mButtonClose->position( mWindowDecoration->size().getWidth() - mBorderRight->size().getWidth() - mButtonClose->size().getWidth() + mButtonsPositionFixer.x, yPos );
	}

	if ( NULL != mButtonMaximize ) {
		yPos = mWindowDecoration->size().getHeight() / 2 - mButtonMaximize->size().getHeight() / 2 + mButtonsPositionFixer.y;

		if ( NULL != mButtonClose ) {
			mButtonMaximize->position( mButtonClose->position().x - mButtonsSeparation - mButtonMaximize->size().getWidth(), yPos );
		} else {
			mButtonMaximize->position( mWindowDecoration->size().getWidth() - mBorderRight->size().getWidth() - mButtonMaximize->size().getWidth() + mButtonsPositionFixer.x, yPos );
		}
	}

	if ( NULL != mButtonMinimize ) {
		yPos = mWindowDecoration->size().getHeight() / 2 - mButtonMinimize->size().getHeight() / 2 + mButtonsPositionFixer.y;

		if ( NULL != mButtonMaximize ) {
			mButtonMinimize->position( mButtonMaximize->position().x - mButtonsSeparation - mButtonMinimize->size().getWidth(), yPos );
		} else {
			if ( NULL != mButtonClose ) {
				mButtonMinimize->position( mButtonClose->position().x - mButtonsSeparation - mButtonMinimize->size().getWidth(), yPos );
			} else {
				mButtonMinimize->position( mWindowDecoration->size().getWidth() - mBorderRight->size().getWidth() - mButtonMinimize->size().getWidth() + mButtonsPositionFixer.x, yPos );
			}
		}
	}

	fixTitleSize();
}

Uint32 UIWindow::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgFocus:
		{
			toFront();
			break;
		}
		case UIMessage::MsgMouseDown:
		{
			doResize( Msg );
			break;
		}
		case UIMessage::MsgWindowResize:
		{
			if ( isModal() && NULL != mModalCtrl ) {
				mModalCtrl->size( UIManager::instance()->mainControl()->size() );
			}

			break;
		}
		case UIMessage::MsgMouseExit:
		{
			UIManager::instance()->setCursor( EE_CURSOR_ARROW );
			break;
		}
		case UIMessage::MsgDragStart:
		{
			UIManager::instance()->setCursor( EE_CURSOR_HAND );
			break;
		}
		case UIMessage::MsgDragEnd:
		{
			UIManager::instance()->setCursor( EE_CURSOR_ARROW );
			break;
		}
	}

	return UIComplexControl::onMessage( Msg );
}

void UIWindow::doResize ( const UIMessage * Msg ) {
	if ( NULL == mWindowDecoration )
		return;

	if (	!( mWinFlags & UI_WIN_RESIZEABLE ) ||
			!( Msg->getFlags() & EE_BUTTON_LMASK ) ||
			RESIZE_NONE != mResizeType ||
			( UIManager::instance()->lastPressTrigger() & EE_BUTTON_LMASK )
	)
		return;

	decideResizeType( Msg->getSender() );
}

void UIWindow::decideResizeType( UIControl * Control ) {
	Vector2i Pos = UIManager::instance()->getMousePos();

	worldToControl( Pos );

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->size().getWidth() ) {
			tryResize( RESIZE_TOPLEFT );
		} else if ( Pos.x >= ( mSize.getWidth() - mBorderRight->size().getWidth() ) ) {
			tryResize( RESIZE_TOPRIGHT );
		} else if ( Pos.y <= mBorderBottom->size().getHeight() ) {
			if ( Pos.x < mMinCornerDistance ) {
				tryResize( RESIZE_TOPLEFT );
			} else if ( Pos.x > mSize.getWidth() - mMinCornerDistance ) {
				tryResize( RESIZE_TOPRIGHT );
			} else {
				tryResize( RESIZE_TOP );
			}
		}
	} else if ( Control == mBorderBottom ) {
		if ( Pos.x < mMinCornerDistance ) {
			tryResize( RESIZE_LEFTBOTTOM );
		} else if ( Pos.x > mSize.getWidth() - mMinCornerDistance ) {
			tryResize( RESIZE_RIGHTBOTTOM );
		} else {
			tryResize( RESIZE_BOTTOM );
		}
	} else if ( Control == mBorderLeft )  {
		if ( Pos.y >= mSize.getHeight() - mMinCornerDistance ) {
			tryResize( RESIZE_LEFTBOTTOM );
		} else {
			tryResize( RESIZE_LEFT );
		}
	} else if ( Control == mBorderRight ) {
		if ( Pos.y >= mSize.getHeight() - mMinCornerDistance ) {
			tryResize( RESIZE_RIGHTBOTTOM );
		} else {
			tryResize( RESIZE_RIGHT );
		}
	}
}

void UIWindow::tryResize( const UI_RESIZE_TYPE& Type ) {
	if ( RESIZE_NONE != mResizeType )
		return;

	dragEnable( false );

	Vector2i Pos = UIManager::instance()->getMousePos();

	worldToControl( Pos );
	
	mResizeType = Type;
	
	switch ( mResizeType )
	{
		case RESIZE_RIGHT:
		{
			mResizePos.x = mSize.getWidth() - Pos.x;
			break;
		}
		case RESIZE_LEFT:
		{
			mResizePos.x = Pos.x;
			break;
		}
		case RESIZE_TOP:
		{
			mResizePos.y = Pos.y;
			break;
		}
		case RESIZE_BOTTOM:
		{
			mResizePos.y = mSize.getHeight() - Pos.y;
			break;
		}
		case RESIZE_RIGHTBOTTOM:
		{
			mResizePos.x = mSize.getWidth() - Pos.x;
			mResizePos.y = mSize.getHeight() - Pos.y;
			break;
		}
		case RESIZE_LEFTBOTTOM:
		{
			mResizePos.x = Pos.x;
			mResizePos.y = mSize.getHeight() - Pos.y;
			break;
		}
		case RESIZE_TOPLEFT:
		{
			mResizePos.x = Pos.x;
			mResizePos.y = Pos.y;
			break;
		}
		case RESIZE_TOPRIGHT:
		{
			mResizePos.y = Pos.y;
			mResizePos.x = mSize.getWidth() - Pos.x;
			break;
		}
		case RESIZE_NONE:
		{
		}
	}
}

void UIWindow::endResize() {
	mResizeType = RESIZE_NONE;
}

void UIWindow::updateResize() {
	if ( RESIZE_NONE == mResizeType )
		return;

	if ( !( UIManager::instance()->pressTrigger() & EE_BUTTON_LMASK ) ) {
		endResize();
		dragEnable( true );
		return;
	}

	Vector2i Pos = UIManager::instance()->getMousePos();

	worldToControl( Pos );

	switch ( mResizeType ) {
		case RESIZE_RIGHT:
		{
			internalSize( Pos.x + mResizePos.x, mSize.getHeight() );
			break;
		}
		case RESIZE_BOTTOM:
		{
			internalSize( mSize.getWidth(), Pos.y + mResizePos.y );
			break;
		}
		case RESIZE_LEFT:
		{
			Pos.x -= mResizePos.x;
			UIControl::position( mPos.x + Pos.x, mPos.y );
			internalSize( mSize.getWidth() - Pos.x, mSize.getHeight() );
			break;
		}
		case RESIZE_TOP:
		{
			Pos.y -= mResizePos.y;
			UIControl::position( mPos.x, mPos.y + Pos.y );
			internalSize( mSize.getWidth(), mSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_RIGHTBOTTOM:
		{
			Pos += mResizePos;
			internalSize( Pos.x, Pos.y );
			break;
		}
		case RESIZE_TOPLEFT:
		{
			Pos -= mResizePos;
			UIControl::position( mPos.x + Pos.x, mPos.y + Pos.y );
			internalSize( mSize.getWidth() - Pos.x, mSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_TOPRIGHT:
		{
			Pos.y -= mResizePos.y;
			Pos.x += mResizePos.x;
			UIControl::position( mPos.x, mPos.y + Pos.y );
			internalSize( Pos.x, mSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_LEFTBOTTOM:
		{
			Pos.x -= mResizePos.x;
			Pos.y += mResizePos.y;
			UIControl::position( mPos.x + Pos.x, mPos.y );
			internalSize( mSize.getWidth() - Pos.x, Pos.y );
			break;
		}
		case RESIZE_NONE:
		{
		}
	}
}

void UIWindow::internalSize( const Int32& w, const Int32& h ) {
	internalSize( Sizei( w, h ) );
}

void UIWindow::internalSize( Sizei Size ) {
	if ( Size.x < mMinWindowSize.x || Size.y < mMinWindowSize.y ) {
		if ( Size.x < mMinWindowSize.x && Size.y < mMinWindowSize.y ) {
			Size = mMinWindowSize;
		} else if ( Size.x < mMinWindowSize.x ) {
			Size.x = mMinWindowSize.x;
		} else {
			Size.y = mMinWindowSize.y;
		}
	}

	if ( Size != mSize ) {
		mSize = Size;
		onSizeChange();
	}
}

void UIWindow::draw() {
	UIComplexControl::draw();

	if ( mWinFlags & UI_WIN_DRAW_SHADOW ) {
		Primitives P;
		P.forceDraw( false );

		ColorA BeginC( 0, 0, 0, 25 * ( alpha() / (Float)255 ) );
		ColorA EndC( 0, 0, 0, 0 );
		Float SSize = 16.f;

		Vector2i ShadowPos = mScreenPos + Vector2i( 0, 16 );

		P.drawRectangle( Rectf( Vector2f( ShadowPos.x, ShadowPos.y ), Sizef( mSize.getWidth(), mSize.getHeight() ) ), BeginC, BeginC, BeginC, BeginC );

		P.drawRectangle( Rectf( Vector2f( ShadowPos.x, ShadowPos.y - SSize ), Sizef( mSize.getWidth(), SSize ) ), EndC, BeginC, BeginC, EndC );

		P.drawRectangle( Rectf( Vector2f( ShadowPos.x - SSize, ShadowPos.y ), Sizef( SSize, mSize.getHeight() ) ), EndC, EndC, BeginC, BeginC );

		P.drawRectangle( Rectf( Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y ), Sizef( SSize, mSize.getHeight() ) ), BeginC, BeginC, EndC, EndC );

		P.drawRectangle( Rectf( Vector2f( ShadowPos.x, ShadowPos.y + mSize.getHeight() ), Sizef( mSize.getWidth(), SSize ) ), BeginC, EndC, EndC, BeginC );

		P.drawTriangle( Triangle2f( Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y ), Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y - SSize ), Vector2f( ShadowPos.x + mSize.getWidth() + SSize, ShadowPos.y ) ), BeginC, EndC, EndC );

		P.drawTriangle( Triangle2f( Vector2f( ShadowPos.x, ShadowPos.y ), Vector2f( ShadowPos.x, ShadowPos.y - SSize ), Vector2f( ShadowPos.x - SSize, ShadowPos.y ) ), BeginC, EndC, EndC );

		P.drawTriangle( Triangle2f( Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y + mSize.getHeight() ), Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y + mSize.getHeight() + SSize ), Vector2f( ShadowPos.x + mSize.getWidth() + SSize, ShadowPos.y + mSize.getHeight() ) ), BeginC, EndC, EndC );

		P.drawTriangle( Triangle2f( Vector2f( ShadowPos.x, ShadowPos.y + mSize.getHeight() ), Vector2f( ShadowPos.x - SSize, ShadowPos.y + mSize.getHeight() ), Vector2f( ShadowPos.x, ShadowPos.y + mSize.getHeight() + SSize ) ), BeginC, EndC, EndC );

		P.forceDraw( true );
	}
}

void UIWindow::update() {
	resizeCursor();

	UIComplexControl::update();

	updateResize();
}

UIControlAnim * UIWindow::getContainer() const {
	return mContainer;
}

UIComplexControl * UIWindow::getButtonClose() const {
	return mButtonClose;
}

UIComplexControl * UIWindow::getButtonMaximize() const {
	return mButtonMaximize;
}

UIComplexControl * UIWindow::getButtonMinimize() const {
	return mButtonMinimize;
}

bool UIWindow::show() {
	if ( !visible() ) {
		enabled( true );
		visible( true );

		setFocus();

		startAlphaAnim( mBaseAlpha == alpha() ? 0.f : mAlpha, mBaseAlpha, UIThemeManager::instance()->controlsFadeInTime() );

		if ( isModal() ) {
			createModalControl();

			mModalCtrl->enabled( true );
			mModalCtrl->visible( true );
			mModalCtrl->toFront();

			toFront();
		}

		return true;
	}

	return false;
}

bool UIWindow::Hide() {
	if ( visible() ) {
		if ( UIThemeManager::instance()->defaultEffectsEnabled() ) {
			disableFadeOut( UIThemeManager::instance()->controlsFadeOutTime() );
		} else {
			enabled( false );
			visible( false );
		}

		UIManager::instance()->mainControl()->setFocus();

		if ( NULL != mModalCtrl ) {
			mModalCtrl->enabled( false );
			mModalCtrl->visible( false );
		}

		return true;
	}

	return false;
}

void UIWindow::onAlphaChange() {
	if ( mWinFlags & UI_WIN_SHARE_ALPHA_WITH_CHILDS ) {
		UIControlAnim * AnimChild;
		UIControl * CurChild = mChild;

		while ( NULL != CurChild ) {
			if ( CurChild->isAnimated() ) {
				AnimChild = reinterpret_cast<UIControlAnim*> ( CurChild );
				AnimChild->alpha( mAlpha );
			}

			CurChild = CurChild->nextGet();
		}
	}

	UIComplexControl::onAlphaChange();
}

void UIWindow::baseAlpha( const Uint8& Alpha ) {
	if ( mAlpha == mBaseAlpha ) {
		UIControlAnim::alpha( Alpha );
	}

	mBaseAlpha = Alpha;
}

const Uint8& UIWindow::baseAlpha() const {
	return mBaseAlpha;
}

void UIWindow::title( const String& Text ) {
	if ( NULL == mTitle ) {
		UITextBox::CreateParams Params;
		Params.setParent( this );
		Params.Flags		= UI_CLIP_ENABLE | UI_VALIGN_CENTER;
		Params.FontColor	= mTitleFontColor;

		if ( mFlags & UI_HALIGN_CENTER )
			Params.Flags |= UI_HALIGN_CENTER;

		if ( mFlags & UI_DRAW_SHADOW )
			Params.Flags |= UI_DRAW_SHADOW;

		mTitle = eeNew( UITextBox, ( Params ) );
		mTitle->enabled( false );
		mTitle->visible( true );
	}

	mTitle->text( Text );

	fixTitleSize();
}

void UIWindow::fixTitleSize() {
	if ( NULL != mWindowDecoration && NULL != mTitle ) {
		mTitle->size( mWindowDecoration->size().getWidth() - mBorderLeft->size().getWidth() - mBorderRight->size().getWidth(), mWindowDecoration->size().getHeight() );
		mTitle->position( mBorderLeft->size().getWidth(), 0 );
	}
}

String UIWindow::title() const {
	if ( NULL != mTitle )
		return mTitle->text();

	return String();
}

UITextBox * UIWindow::titleTextBox() const {
	return mTitle;
}

void UIWindow::maximize() {
	UIControl * Ctrl = UIManager::instance()->mainControl();

	if ( Ctrl->size() == mSize ) {
		position( mNonMaxPos );
		internalSize( mNonMaxSize );
	} else {
		mNonMaxPos	= mPos;
		mNonMaxSize = mSize;

		position( 0, 0 );
		internalSize( UIManager::instance()->mainControl()->size() );
	}
}

Uint32 UIWindow::onMouseDoubleClick( const Vector2i &Pos, const Uint32 Flags ) {
	if ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( NULL != mButtonMaximize ) && ( Flags & EE_BUTTON_LMASK ) ) {
		buttonMaximizeClick( NULL );
	}

	return 1;
}

Uint32 UIWindow::onKeyDown( const UIEventKey &Event ) {
	checkShortcuts( Event.getKeyCode(), Event.getMod() );

	return UIComplexControl::onKeyDown( Event );
}

void UIWindow::checkShortcuts( const Uint32& KeyCode, const Uint32& Mod ) {
	for ( KeyboardShortcuts::iterator it = mKbShortcuts.begin(); it != mKbShortcuts.end(); it++ ) {
		KeyboardShortcut kb = (*it);

		if ( KeyCode == kb.KeyCode && ( Mod & kb.Mod ) ) {
			UIManager::instance()->sendMouseUp( kb.Button, Vector2i(0,0), EE_BUTTON_LMASK );
			UIManager::instance()->sendMouseClick( kb.Button, Vector2i(0,0), EE_BUTTON_LMASK );
		}
	}
}

UIWindow::KeyboardShortcuts::iterator UIWindow::existsShortcut( const Uint32& KeyCode, const Uint32& Mod ) {
	for ( KeyboardShortcuts::iterator it = mKbShortcuts.begin(); it != mKbShortcuts.end(); it++ ) {
		if ( (*it).KeyCode == KeyCode && (*it).Mod == Mod )
			return it;
	}

	return mKbShortcuts.end();
}

bool UIWindow::addShortcut( const Uint32& KeyCode, const Uint32& Mod, UIPushButton * Button ) {
	if ( inParentTreeOf( Button ) && mKbShortcuts.end() == existsShortcut( KeyCode, Mod ) ) {
		mKbShortcuts.push_back( KeyboardShortcut( KeyCode, Mod, Button ) );

		return true;
	}

	return false;
}

bool UIWindow::removeShortcut( const Uint32& KeyCode, const Uint32& Mod ) {
	KeyboardShortcuts::iterator it = existsShortcut( KeyCode, Mod );

	if ( mKbShortcuts.end() != it ) {
		mKbShortcuts.erase( it );

		return true;
	}

	return false;
}

bool UIWindow::isMaximixable() {
	return 0 != ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( mWinFlags & UI_WIN_MAXIMIZE_BUTTON ) );
}

bool UIWindow::isModal() {
	return 0 != ( mWinFlags & UI_WIN_MODAL );
}

UIControlAnim * UIWindow::getModalControl() const {
	return mModalCtrl;
}

void UIWindow::resizeCursor() {
	UIManager * Man = UIManager::instance();

	if ( !isMouseOverMeOrChilds() || !Man->useGlobalCursors() || ( mWinFlags & UI_WIN_NO_BORDER ) || !( mWinFlags & UI_WIN_RESIZEABLE ) )
		return;

	Vector2i Pos = Man->getMousePos();

	worldToControl( Pos );

	const UIControl * Control = Man->overControl();

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->size().getWidth() ) {
			Man->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_TOPLEFT
		} else if ( Pos.x >= ( mSize.getWidth() - mBorderRight->size().getWidth() ) ) {
			Man->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_TOPRIGHT
		} else if ( Pos.y <= mBorderBottom->size().getHeight() ) {
			if ( Pos.x < mMinCornerDistance ) {
				Man->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_TOPLEFT
			} else if ( Pos.x > mSize.getWidth() - mMinCornerDistance ) {
				Man->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_TOPRIGHT
			} else {
				Man->setCursor( EE_CURSOR_SIZENS ); // RESIZE_TOP
			}
		} else if ( !( UIManager::instance()->pressTrigger() & EE_BUTTON_LMASK ) ) {
			Man->setCursor( EE_CURSOR_ARROW );
		}
	} else if ( Control == mBorderBottom ) {
		if ( Pos.x < mMinCornerDistance ) {
			Man->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_LEFTBOTTOM
		} else if ( Pos.x > mSize.getWidth() - mMinCornerDistance ) {
			Man->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			Man->setCursor( EE_CURSOR_SIZENS ); // RESIZE_BOTTOM
		}
	} else if ( Control == mBorderLeft )  {
		if ( Pos.y >= mSize.getHeight() - mMinCornerDistance ) {
			Man->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_LEFTBOTTOM
		} else {
			Man->setCursor( EE_CURSOR_SIZEWE ); // RESIZE_LEFT
		}
	} else if ( Control == mBorderRight ) {
		if ( Pos.y >= mSize.getHeight() - mMinCornerDistance ) {
			Man->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			Man->setCursor( EE_CURSOR_SIZEWE ); // RESIZE_RIGHT
		}
	}
}

}}
