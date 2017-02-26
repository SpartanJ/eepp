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
	mContainer->setEnabled( true );
	mContainer->setVisible( true );
	mContainer->setSize( mSize );
	mContainer->addEventListener( UIEvent::EventOnPosChange, cb::Make1( this, &UIWindow::onContainerPosChange ) );

	if ( !( mWinFlags & UI_WIN_NO_BORDER ) ) {
		UIControlAnim::CreateParams tParams;
		tParams.setParent( this );

		mWindowDecoration = eeNew( UIControlAnim, ( tParams ) );
		mWindowDecoration->setVisible( true );
		mWindowDecoration->setEnabled( false );

		mBorderLeft		= eeNew( UIControlAnim, ( tParams ) );
		mBorderLeft->setEnabled( true );
		mBorderLeft->setVisible( true );

		mBorderRight	= eeNew( UIControlAnim, ( tParams ) );
		mBorderRight->setEnabled( true );
		mBorderRight->setVisible( true );

		mBorderBottom	= eeNew( UIControlAnim, ( tParams ) );
		mBorderBottom->setEnabled( true );
		mBorderBottom->setVisible( true );

		if ( mWinFlags & UI_WIN_DRAGABLE_CONTAINER )
			mContainer->setDragEnabled( true );

		UIComplexControl::CreateParams ButtonParams;
		ButtonParams.setParent( this );

		if ( mWinFlags & UI_WIN_CLOSE_BUTTON ) {
			mButtonClose = eeNew( UIComplexControl, ( ButtonParams ) );
			mButtonClose->setVisible( true );
			mButtonClose->setEnabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonClose->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIWindow::onButtonCloseClick ) );
			}
		}

		if ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( mWinFlags & UI_WIN_MAXIMIZE_BUTTON ) ) {
			mButtonMaximize = eeNew( UIComplexControl, ( ButtonParams ) );
			mButtonMaximize->setVisible( true );
			mButtonMaximize->setEnabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonMaximize->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIWindow::onButtonMaximizeClick ) );
			}
		}

		if ( mWinFlags & UI_WIN_MINIMIZE_BUTTON ) {
			mButtonMinimize = eeNew( UIComplexControl, ( ButtonParams ) );
			mButtonMinimize->setVisible( true );
			mButtonMinimize->setEnabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonMinimize->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIWindow::onButtonMinimizeClick ) );
			}
		}

		setDragEnabled( true );
	}

	if ( isModal() ) {
		createModalControl();
	}

	setAlpha( mBaseAlpha );

	applyDefaultTheme();
}

UIWindow::~UIWindow() {
	UIManager::instance()->windowRemove( this );

	UIManager::instance()->setFocusLastWindow( this );

	sendCommonEvent( UIEvent::EventOnWindowClose );
}

void UIWindow::createModalControl() {
	UIControl * Ctrl = UIManager::instance()->getMainControl();

	if ( NULL == mModalCtrl ) {
		mModalCtrl = eeNew( UIControlAnim, ( UIControlAnim::CreateParams( Ctrl , Vector2i(0,0), Ctrl->getSize(), UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) );
	} else {
		mModalCtrl->setPosition( 0, 0 );
		mModalCtrl->setSize( Ctrl->getSize() );
	}

	disableByModal();
}

void UIWindow::enableByModal() {
	if ( isModal() ) {
		UIControl * CtrlChild = UIManager::instance()->getMainControl()->getFirstChild();

		while ( NULL != CtrlChild )
		{
			if ( CtrlChild != mModalCtrl &&
				 CtrlChild != this &&
				 CtrlChild->getControlFlags() & UI_CTRL_FLAG_DISABLED_BY_MODAL_WINDOW )
			{
				CtrlChild->setEnabled( true );
				CtrlChild->writeCtrlFlag( UI_CTRL_FLAG_DISABLED_BY_MODAL_WINDOW, 0 );
			}

			CtrlChild = CtrlChild->getNextControl();
		}
	}
}

void UIWindow::disableByModal() {
	if ( isModal() ) {
		UIControl * CtrlChild = UIManager::instance()->getMainControl()->getFirstChild();

		while ( NULL != CtrlChild )
		{
			if ( CtrlChild != mModalCtrl &&
				 CtrlChild != this &&
				 CtrlChild->isEnabled() )
			{
				CtrlChild->setEnabled( false );
				CtrlChild->writeCtrlFlag( UI_CTRL_FLAG_DISABLED_BY_MODAL_WINDOW, 1 );
			}

			CtrlChild = CtrlChild->getNextControl();
		}
	}
}

Uint32 UIWindow::getType() const {
	return UI_TYPE_WINDOW;
}

bool UIWindow::isType( const Uint32& type ) const {
	return UIWindow::getType() == type ? true : UIComplexControl::isType( type );
}

void UIWindow::onContainerPosChange( const UIEvent * Event ) {
	Vector2i PosDiff = mContainer->getPosition() - Vector2i( mBorderLeft->getSize().getWidth(), mWindowDecoration->getSize().getHeight() );

	if ( PosDiff.x != 0 || PosDiff.y != 0 ) {
		mContainer->setPosition( mBorderLeft->getSize().getWidth(), mWindowDecoration->getSize().getHeight() );

		setPosition( mPos + PosDiff );
	}
}

void UIWindow::onButtonCloseClick( const UIEvent * Event ) {
	CloseWindow();

	sendCommonEvent( UIEvent::EventOnWindowCloseClick );
}

void UIWindow::CloseWindow() {
	if ( NULL != mButtonClose )
		mButtonClose->setEnabled( false );

	if ( NULL != mButtonMaximize )
		mButtonMaximize->setEnabled( false );

	if ( NULL != mButtonMinimize )
		mButtonMinimize->setEnabled( false );

	if ( NULL != mModalCtrl ) {
		mModalCtrl->close();
		mModalCtrl = NULL;
	}

	if ( Time::Zero != UIThemeManager::instance()->getControlsFadeOutTime() )
		closeFadeOut( UIThemeManager::instance()->getControlsFadeOutTime() );
	else
		close();
}

void UIWindow::close() {
	UIComplexControl::close();

	enableByModal();
}

void UIWindow::onButtonMaximizeClick( const UIEvent * Event ) {
	maximize();

	sendCommonEvent( UIEvent::EventOnWindowMaximizeClick );
}

void UIWindow::onButtonMinimizeClick( const UIEvent * Event ) {
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
			mButtonClose->setSize( mButtonClose->getSkinSize() );
		}

		if ( NULL != mButtonMaximize ) {
			mButtonMaximize->setThemeControl( Theme, "winmax" );
			mButtonMaximize->setSize( mButtonMaximize->getSkinSize() );
		}

		if ( NULL != mButtonMinimize ) {
			mButtonMinimize->setThemeControl( Theme, "winmin" );
			mButtonMinimize->setSize( mButtonMinimize->getSkinSize() );
		}

		getMinWinSize();
		fixChildsSize();
	}
}

void UIWindow::getMinWinSize() {
	if ( NULL == mWindowDecoration || ( mMinWindowSize.x != 0 && mMinWindowSize.y != 0 ) )
		return;

	Sizei tSize;

	tSize.x = mBorderLeft->getSize().getWidth() + mBorderRight->getSize().getWidth() - mButtonsPositionFixer.x;
	tSize.y = mWindowDecoration->getSize().getHeight() + mBorderBottom->getSize().getHeight();

	if ( NULL != mButtonClose )
		tSize.x += mButtonClose->getSize().getWidth();

	if ( NULL != mButtonMaximize )
		tSize.x += mButtonMaximize->getSize().getWidth();

	if ( NULL != mButtonMinimize )
		tSize.x += mButtonMinimize->getSize().getWidth();

	if ( mMinWindowSize.x < tSize.x )
		mMinWindowSize.x = tSize.x;

	if ( mMinWindowSize.y < tSize.y )
		mMinWindowSize.y = tSize.y;
}

void UIWindow::applyMinWinSize() {
	if ( mSize.x < mMinWindowSize.x && mSize.y < mMinWindowSize.y ) {
		setSize( mMinWindowSize );
	} else if ( mSize.x < mMinWindowSize.x ) {
		setSize( Sizei( mMinWindowSize.x, mSize.y ) );
	} else if ( mSize.y < mMinWindowSize.y ) {
		setSize( Sizei( mSize.x, mMinWindowSize.y ) );
	}
}

void UIWindow::onSizeChange() {
	if ( mSize.x < mMinWindowSize.x || mSize.y < mMinWindowSize.y ) {
		if ( mSize.x < mMinWindowSize.x && mSize.y < mMinWindowSize.y ) {
			setSize( mMinWindowSize );
		} else if ( mSize.x < mMinWindowSize.x ) {
			setSize( Sizei( mMinWindowSize.x, mSize.y ) );
		} else {
			setSize( Sizei( mSize.x, mMinWindowSize.y ) );
		}
	} else {
		fixChildsSize();

		UIComplexControl::onSizeChange();
	}
}

UIControl * UIWindow::setSize( const Sizei& Size ) {
	if ( NULL != mWindowDecoration ) {
		Sizei size = Size;

		size.x += mBorderLeft->getSize().getWidth() + mBorderRight->getSize().getWidth();
		size.y += mWindowDecoration->getSize().getHeight() + mBorderBottom->getSize().getHeight();

		UIComplexControl::setSize( size );
	} else {
		UIComplexControl::setSize( Size );
	}

	return this;
}

UIControl * UIWindow::setSize( const Int32& Width, const Int32& Height ) {
	setSize( Sizei( Width, Height ) );
	return this;
}

const Sizei& UIWindow::getSize() {
	return UIComplexControl::getSize();
}

void UIWindow::fixChildsSize() {
	if ( mSize.getWidth() < mMinWindowSize.getWidth() || mSize.getHeight() < mMinWindowSize.getHeight() ) {
		internalSize( eemin( mSize.getWidth(), mMinWindowSize.getWidth() ), eemin( mSize.getHeight(), mMinWindowSize.getHeight() ) );
	}

	if ( NULL == mWindowDecoration ) {
		mContainer->setSize( mSize.getWidth(), mSize.getHeight() );
		return;
	}

	Sizei decoSize = mDecoSize;

	if ( mDecoAutoSize ) {
		decoSize = mDecoSize = Sizei( mSize.getWidth(), mWindowDecoration->getSkinSize().getHeight() );
	}

	mWindowDecoration->setSize( mDecoSize );

	if ( mBorderAutoSize ) {
		mBorderBottom->setSize( Sizei( mSize.getWidth(), mBorderBottom->getSkinSize().getHeight() ) );
	} else {
		mBorderBottom->setSize( mSize.getWidth(), mBorderSize.getHeight() );
	}

	Uint32 BorderHeight = mSize.getHeight() - decoSize.getHeight() - mBorderBottom->getSize().getHeight();

	if ( mBorderAutoSize ) {
		mBorderLeft->setSize( Sizei( mBorderLeft->getSkinSize().getWidth(), BorderHeight ) );
		mBorderRight->setSize( Sizei( mBorderRight->getSkinSize().getWidth(), BorderHeight ) );
	} else {
		mBorderLeft->setSize( mBorderSize.getWidth(), BorderHeight );
		mBorderRight->setSize( mBorderSize.getWidth(), BorderHeight );
	}

	mBorderLeft->setPosition( 0, mWindowDecoration->getSize().getHeight() );
	mBorderRight->setPosition( mSize.getWidth() - mBorderRight->getSize().getWidth(), mWindowDecoration->getSize().getHeight() );
	mBorderBottom->setPosition( 0, mSize.getHeight() - mBorderBottom->getSize().getHeight() );

	mContainer->setPosition( mBorderLeft->getSize().getWidth(), mWindowDecoration->getSize().getHeight() );
	mContainer->setSize( mSize.getWidth() - mBorderLeft->getSize().getWidth() - mBorderRight->getSize().getWidth(), mSize.getHeight() - mWindowDecoration->getSize().getHeight() - mBorderBottom->getSize().getHeight() );

	Uint32 yPos;

	if ( NULL != mButtonClose ) {
		yPos = mWindowDecoration->getSize().getHeight() / 2 - mButtonClose->getSize().getHeight() / 2 + mButtonsPositionFixer.y;

		mButtonClose->setPosition( mWindowDecoration->getSize().getWidth() - mBorderRight->getSize().getWidth() - mButtonClose->getSize().getWidth() + mButtonsPositionFixer.x, yPos );
	}

	if ( NULL != mButtonMaximize ) {
		yPos = mWindowDecoration->getSize().getHeight() / 2 - mButtonMaximize->getSize().getHeight() / 2 + mButtonsPositionFixer.y;

		if ( NULL != mButtonClose ) {
			mButtonMaximize->setPosition( mButtonClose->getPosition().x - mButtonsSeparation - mButtonMaximize->getSize().getWidth(), yPos );
		} else {
			mButtonMaximize->setPosition( mWindowDecoration->getSize().getWidth() - mBorderRight->getSize().getWidth() - mButtonMaximize->getSize().getWidth() + mButtonsPositionFixer.x, yPos );
		}
	}

	if ( NULL != mButtonMinimize ) {
		yPos = mWindowDecoration->getSize().getHeight() / 2 - mButtonMinimize->getSize().getHeight() / 2 + mButtonsPositionFixer.y;

		if ( NULL != mButtonMaximize ) {
			mButtonMinimize->setPosition( mButtonMaximize->getPosition().x - mButtonsSeparation - mButtonMinimize->getSize().getWidth(), yPos );
		} else {
			if ( NULL != mButtonClose ) {
				mButtonMinimize->setPosition( mButtonClose->getPosition().x - mButtonsSeparation - mButtonMinimize->getSize().getWidth(), yPos );
			} else {
				mButtonMinimize->setPosition( mWindowDecoration->getSize().getWidth() - mBorderRight->getSize().getWidth() - mButtonMinimize->getSize().getWidth() + mButtonsPositionFixer.x, yPos );
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
				mModalCtrl->setSize( UIManager::instance()->getMainControl()->getSize() );
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
			( UIManager::instance()->getLastPressTrigger() & EE_BUTTON_LMASK )
	)
		return;

	decideResizeType( Msg->getSender() );
}

void UIWindow::decideResizeType( UIControl * Control ) {
	Vector2i Pos = UIManager::instance()->getMousePos();

	worldToControl( Pos );

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->getSize().getWidth() ) {
			tryResize( RESIZE_TOPLEFT );
		} else if ( Pos.x >= ( mSize.getWidth() - mBorderRight->getSize().getWidth() ) ) {
			tryResize( RESIZE_TOPRIGHT );
		} else if ( Pos.y <= mBorderBottom->getSize().getHeight() ) {
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

	setDragEnabled( false );

	Vector2i Pos = UIManager::instance()->getMousePos();

	worldToControl( Pos );
	
	mResizeType = Type;

	Pos = PixelDensity::dpToPxI( Pos );

	switch ( mResizeType )
	{
		case RESIZE_RIGHT:
		{
			mResizePos.x = mRealSize.getWidth() - Pos.x;
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
			mResizePos.y = mRealSize.getHeight() - Pos.y;
			break;
		}
		case RESIZE_RIGHTBOTTOM:
		{
			mResizePos.x = mRealSize.getWidth() - Pos.x;
			mResizePos.y = mRealSize.getHeight() - Pos.y;
			break;
		}
		case RESIZE_LEFTBOTTOM:
		{
			mResizePos.x = Pos.x;
			mResizePos.y = mRealSize.getHeight() - Pos.y;
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
			mResizePos.x = mRealSize.getWidth() - Pos.x;
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

	if ( !( UIManager::instance()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
		endResize();
		setDragEnabled( true );
		return;
	}

	Vector2i Pos = UIManager::instance()->getMousePos();

	worldToControl( Pos );

	Pos = PixelDensity::dpToPxI( Pos );

	switch ( mResizeType ) {
		case RESIZE_RIGHT:
		{
			internalSize( Pos.x + mResizePos.x, mRealSize.getHeight() );
			break;
		}
		case RESIZE_BOTTOM:
		{
			internalSize( mRealSize.getWidth(), Pos.y + mResizePos.y );
			break;
		}
		case RESIZE_LEFT:
		{
			Pos.x -= mResizePos.x;
			UIControl::setPixelsPosition( mRealPos.x + Pos.x, mRealPos.y );
			internalSize( mRealSize.getWidth() - Pos.x, mRealSize.getHeight() );
			break;
		}
		case RESIZE_TOP:
		{
			Pos.y -= mResizePos.y;
			UIControl::setPixelsPosition( mRealPos.x, mRealPos.y + Pos.y );
			internalSize( mRealSize.getWidth(), mRealSize.getHeight() - Pos.y );
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
			UIControl::setPixelsPosition( mRealPos.x + Pos.x, mRealPos.y + Pos.y );
			internalSize( mRealSize.getWidth() - Pos.x, mRealSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_TOPRIGHT:
		{
			Pos.y -= mResizePos.y;
			Pos.x += mResizePos.x;
			UIControl::setPixelsPosition( mRealPos.x, mRealPos.y + Pos.y );
			internalSize( Pos.x, mRealSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_LEFTBOTTOM:
		{
			Pos.x -= mResizePos.x;
			Pos.y += mResizePos.y;
			UIControl::setPixelsPosition( mRealPos.x + Pos.x, mRealPos.y );
			internalSize( mRealSize.getWidth() - Pos.x, Pos.y );
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
	Sizei realMin = PixelDensity::dpToPxI( mMinWindowSize );

	if ( Size.x < realMin.x || Size.y < realMin.y ) {
		if ( Size.x < realMin.x && Size.y < realMin.y ) {
			Size = realMin;
		} else if ( Size.x < realMin.x ) {
			Size.x = realMin.x;
		} else {
			Size.y = realMin.y;
		}
	}

	if ( Size != mRealSize ) {
		setInternalPixelsSize( Size );
		onSizeChange();
	}
}

void UIWindow::draw() {
	UIComplexControl::draw();

	if ( mWinFlags & UI_WIN_DRAW_SHADOW ) {
		Primitives P;
		P.setForceDraw( false );

		ColorA BeginC( 0, 0, 0, 25 * ( getAlpha() / (Float)255 ) );
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

		P.setForceDraw( true );
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
	if ( !isVisible() ) {
		setEnabled( true );
		setVisible( true );

		setFocus();

		startAlphaAnim( mBaseAlpha == getAlpha() ? 0.f : mAlpha, mBaseAlpha, UIThemeManager::instance()->getControlsFadeInTime() );

		if ( isModal() ) {
			createModalControl();

			mModalCtrl->setEnabled( true );
			mModalCtrl->setVisible( true );
			mModalCtrl->toFront();

			toFront();
		}

		return true;
	}

	return false;
}

bool UIWindow::Hide() {
	if ( isVisible() ) {
		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			disableFadeOut( UIThemeManager::instance()->getControlsFadeOutTime() );
		} else {
			setEnabled( false );
			setVisible( false );
		}

		UIManager::instance()->getMainControl()->setFocus();

		if ( NULL != mModalCtrl ) {
			mModalCtrl->setEnabled( false );
			mModalCtrl->setVisible( false );
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
				AnimChild->setAlpha( mAlpha );
			}

			CurChild = CurChild->getNextControl();
		}
	}

	UIComplexControl::onAlphaChange();
}

void UIWindow::setBaseAlpha( const Uint8& Alpha ) {
	if ( mAlpha == mBaseAlpha ) {
		UIControlAnim::setAlpha( Alpha );
	}

	mBaseAlpha = Alpha;
}

const Uint8& UIWindow::getBaseAlpha() const {
	return mBaseAlpha;
}

void UIWindow::setTitle( const String& Text ) {
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
		mTitle->setEnabled( false );
		mTitle->setVisible( true );
	}

	mTitle->setText( Text );

	fixTitleSize();
}

void UIWindow::fixTitleSize() {
	if ( NULL != mWindowDecoration && NULL != mTitle ) {
		mTitle->setSize( mWindowDecoration->getSize().getWidth() - mBorderLeft->getSize().getWidth() - mBorderRight->getSize().getWidth(), mWindowDecoration->getSize().getHeight() );
		mTitle->setPosition( mBorderLeft->getSize().getWidth(), 0 );
	}
}

String UIWindow::getTitle() const {
	if ( NULL != mTitle )
		return mTitle->getText();

	return String();
}

UITextBox * UIWindow::getTitleTextBox() const {
	return mTitle;
}

void UIWindow::maximize() {
	UIControl * Ctrl = UIManager::instance()->getMainControl();

	if ( Ctrl->getSize() == mSize ) {
		setPixelsPosition( mNonMaxPos );
		internalSize( mNonMaxSize );
	} else {
		mNonMaxPos	= mRealPos;
		mNonMaxSize = mRealSize;

		setPosition( 0, 0 );
		internalSize( UIManager::instance()->getMainControl()->getRealSize() );
	}
}

Uint32 UIWindow::onMouseDoubleClick( const Vector2i &Pos, const Uint32 Flags ) {
	if ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( NULL != mButtonMaximize ) && ( Flags & EE_BUTTON_LMASK ) ) {
		onButtonMaximizeClick( NULL );
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

bool UIWindow::isMaximizable() {
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

	if ( !isMouseOverMeOrChilds() || !Man->getUseGlobalCursors() || ( mWinFlags & UI_WIN_NO_BORDER ) || !( mWinFlags & UI_WIN_RESIZEABLE ) )
		return;

	Vector2i Pos = Man->getMousePos();

	worldToControl( Pos );

	const UIControl * Control = Man->getOverControl();

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->getSize().getWidth() ) {
			Man->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_TOPLEFT
		} else if ( Pos.x >= ( mSize.getWidth() - mBorderRight->getSize().getWidth() ) ) {
			Man->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_TOPRIGHT
		} else if ( Pos.y <= mBorderBottom->getSize().getHeight() ) {
			if ( Pos.x < mMinCornerDistance ) {
				Man->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_TOPLEFT
			} else if ( Pos.x > mSize.getWidth() - mMinCornerDistance ) {
				Man->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_TOPRIGHT
			} else {
				Man->setCursor( EE_CURSOR_SIZENS ); // RESIZE_TOP
			}
		} else if ( !( UIManager::instance()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
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
