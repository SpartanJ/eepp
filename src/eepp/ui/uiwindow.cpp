#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/actions/actions.hpp>

namespace EE { namespace UI {

UIWindow * UIWindow::NewOpt( UIWindow::WindowBaseContainerType type, const UIWindowStyleConfig& windowStyleConfig ) {
	return eeNew( UIWindow, ( type, windowStyleConfig ) );
}

UIWindow * UIWindow::New() {
	return eeNew( UIWindow, ( SIMPLE_LAYOUT ) );
}

UIWindow::UIWindow( UIWindow::WindowBaseContainerType type ) :
	UIWindow( type, NULL != UIThemeManager::instance()->getDefaultTheme() ? UIThemeManager::instance()->getDefaultTheme()->getWindowStyleConfig() : UIWindowStyleConfig() )
{}

UIWindow::UIWindow( UIWindow::WindowBaseContainerType type, const UIWindowStyleConfig& windowStyleConfig ) :
	UIWidget(),
	mFrameBuffer( NULL ),
	mStyleConfig( windowStyleConfig ),
	mWindowDecoration( NULL ),
	mBorderLeft( NULL ),
	mBorderRight( NULL ),
	mBorderBottom( NULL ),
	mButtonClose( NULL ),
	mButtonMinimize( NULL ),
	mButtonMaximize( NULL ),
	mTitle( NULL ),
	mModalCtrl( NULL ),
	mResizeType( RESIZE_NONE ),
	mFrameBufferBound( false )
{
	mNodeFlags |= NODE_FLAG_WINDOW | NODE_FLAG_VIEW_DIRTY;

	setHorizontalAlign( UI_HALIGN_CENTER );

	if ( NULL != getUISceneNode() )
		getUISceneNode()->windowAdd( this );

	switch ( type ) {
		case LINEAR_LAYOUT:
			mContainer		= UILinearLayout::New();
			break;
		case RELATIVE_LAYOUT:
			mContainer		= UIRelativeLayout::New();
			break;
		case SIMPLE_LAYOUT:
		default:
			mContainer		= UIWidget::New();
			break;
	}

	mContainer->setLayoutSizeRules( FIXED, FIXED );
	mContainer->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
	mContainer->setParent( this );
	mContainer->clipEnable();
	mContainer->enableReportSizeChangeToChilds();
	mContainer->setSize( mDpSize );
	mContainer->addEventListener( Event::OnPositionChange, cb::Make1( this, &UIWindow::onContainerPositionChange ) );

	updateWinFlags();

	setAlpha( mStyleConfig.BaseAlpha );

	applyDefaultTheme();
}

UIWindow::~UIWindow() {
	if ( NULL != getUISceneNode() && !SceneManager::instance()->isShootingDown() ) {
		getUISceneNode()->windowRemove( this );

		getUISceneNode()->setFocusLastWindow( this );
	}

	sendCommonEvent( Event::OnWindowClose );

	onClose();

	eeSAFE_DELETE( mFrameBuffer );
}

void UIWindow::onContainerPositionChange( const Event * Event ) {
	if ( NULL == mContainer )
		return;

	Vector2f PosDiff = mContainer->getPosition() - Vector2f( NULL != mBorderLeft ? mBorderLeft->getSize().getWidth() + mPadding.Left : mPadding.Left, NULL != mWindowDecoration ? mWindowDecoration->getSize().getHeight() + mPadding.Top : mPadding.Top );

	if ( PosDiff.x != 0 || PosDiff.y != 0 ) {
		mContainer->setPosition( NULL != mBorderLeft ? mBorderLeft->getSize().getWidth() : 0, NULL != mWindowDecoration ? mWindowDecoration->getSize().getHeight() : 0 );

		setPosition( mDpPos + PosDiff );
	}
}

void UIWindow::updateWinFlags() {
	bool needsUpdate = false;

	writeCtrlFlag( NODE_FLAG_FRAME_BUFFER, ( mStyleConfig.WinFlags & UI_WIN_FRAME_BUFFER ) ? 1 : 0 );

	if ( ( mStyleConfig.WinFlags & UI_WIN_FRAME_BUFFER ) ) {
		if ( NULL == mFrameBuffer )
			createFrameBuffer();
	} else {
		eeSAFE_DELETE( mFrameBuffer );
	}

	if ( NULL != mContainer && ( mStyleConfig.WinFlags & UI_WIN_DRAGABLE_CONTAINER ) ) {
		mContainer->setDragEnabled( true );
	} else {
		setDragEnabled( false );
	}

	if ( !( mStyleConfig.WinFlags & UI_WIN_NO_BORDER ) ) {
		if ( NULL == mWindowDecoration ) {
			mWindowDecoration = UINode::New();
			mWindowDecoration->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mWindowDecoration->setParent( this );
		mWindowDecoration->setVisible( true );
		mWindowDecoration->setEnabled( false );

		if ( NULL == mBorderLeft ) {
			mBorderLeft		= UINode::New();
			mBorderLeft->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mBorderLeft->setParent( this );
		mBorderLeft->setEnabled( true );
		mBorderLeft->setVisible( true );

		if ( NULL == mBorderRight ) {
			mBorderRight	= UINode::New();
			mBorderRight->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mBorderRight->setParent( this );
		mBorderRight->setEnabled( true );
		mBorderRight->setVisible( true );

		if ( NULL == mBorderBottom ) {
			mBorderBottom	= UINode::New();
			mBorderBottom->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mBorderBottom->setParent( this );
		mBorderBottom->setEnabled( true );
		mBorderBottom->setVisible( true );

		if ( mStyleConfig.WinFlags & UI_WIN_CLOSE_BUTTON ) {
			if ( NULL == mButtonClose ) {
				mButtonClose = UINode::New();
				mButtonClose->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
				needsUpdate = true;
			}

			mButtonClose->setParent( this );
			mButtonClose->setVisible( true );
			mButtonClose->setEnabled( true );
		} else if ( NULL != mButtonClose ) {
			mButtonClose->setVisible( false )->setEnabled( false )->close();
			mButtonClose = NULL;
		}

		if ( isMaximizable() ) {
			if ( NULL == mButtonMaximize ) {
				mButtonMaximize = UINode::New();
				mButtonMaximize->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
				needsUpdate = true;
			}

			mButtonMaximize->setParent( this );
			mButtonMaximize->setVisible( true );
			mButtonMaximize->setEnabled( true );
		} else if ( NULL != mButtonMaximize ) {
			mButtonMaximize->setVisible( false )->setEnabled( false )->close();
			mButtonMaximize = NULL;
		}

		if ( mStyleConfig.WinFlags & UI_WIN_MINIMIZE_BUTTON ) {
			if ( NULL == mButtonMinimize ) {
				mButtonMinimize = UINode::New();
				mButtonMinimize->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
				needsUpdate = true;
			}

			mButtonMinimize->setParent( this );
			mButtonMinimize->setVisible( true );
			mButtonMinimize->setEnabled( true );
		} else if ( NULL != mButtonMinimize ) {
			mButtonMinimize->setVisible( false )->setEnabled( false )->close();
			mButtonMinimize = NULL;
		}

		if ( NULL != mTitle ) {
			mTitle->setVisible( true );
			mTitle->toFront();
		}

		setDragEnabled( true );
	} else {
		if ( NULL != mWindowDecoration ) {
			mWindowDecoration->close();
			mWindowDecoration = NULL;
		}

		if ( NULL != mBorderLeft ) {
			mBorderLeft->close();
			mBorderLeft = NULL;
		}

		if ( NULL != mBorderRight ) {
			mBorderRight->close();
			mBorderRight = NULL;
		}

		if ( NULL != mBorderBottom ) {
			mBorderBottom->close();
			mBorderBottom = NULL;
		}

		if ( NULL != mButtonClose ) {
			mButtonClose->close();
			mButtonClose = NULL;
		}

		if ( NULL != mButtonMaximize ) {
			mButtonMaximize->close();
			mButtonMaximize = NULL;
		}

		if ( NULL != mButtonMinimize ) {
			mButtonMinimize->close();
			mButtonMinimize = NULL;
		}

		if ( NULL != mButtonClose ) {
			mButtonClose->close();
			mButtonClose = NULL;
		}

		if ( NULL != mContainer )
			mContainer->setPosition( 0, 0 );

		if ( NULL != mTitle )
			mTitle->setVisible( false );

		fixChildsSize();
	}

	updateDrawInvalidator( true );

	if ( isModal() ) {
		createModalControl();
	}

	if ( needsUpdate ) {
		applyDefaultTheme();
	}
}

void UIWindow::createFrameBuffer() {
	eeSAFE_DELETE( mFrameBuffer );
	Sizei fboSize( getFrameBufferSize() );
	if ( fboSize.getWidth() < 1 ) fboSize.setWidth(1);
	if ( fboSize.getHeight() < 1 ) fboSize.setHeight(1);
	mFrameBuffer = FrameBuffer::New( fboSize.getWidth(), fboSize.getHeight(), true, false, ( mStyleConfig.WinFlags & UI_WIN_COLOR_BUFFER ) ? true : false );

	// Frame buffer failed to create?
	if ( !mFrameBuffer->created() ) {
		eeSAFE_DELETE( mFrameBuffer );
	}
}

void UIWindow::drawFrameBuffer() {
	if ( NULL != mFrameBuffer ) {
		if ( mFrameBuffer->hasColorBuffer() ) {
			mFrameBuffer->draw( Rect( 0, 0, mSize.getWidth(), mSize.getHeight() ), Rect( mScreenPos.x, mScreenPos.y, mScreenPos.x + mSize.getWidth(), mScreenPos.y + mSize.getHeight() ) );
		} else {
			TextureRegion textureRegion( mFrameBuffer->getTexture()->getId(), Rect( 0, 0, mSize.getWidth(), mSize.getHeight() ) );
			textureRegion.draw( mScreenPosi.x, mScreenPosi.y, Color::White, getRotation(), getScale() );
		}
	}
}

void UIWindow::drawHighlightInvalidation() {
	if ( ( mNodeFlags & NODE_FLAG_VIEW_DIRTY ) && NULL != mSceneNode && mSceneNode->getHighlightInvalidation() ) {
		UIWidget::matrixSet();

		Primitives P;
		P.setFillMode( DRAW_LINE );
		P.setBlendMode( getBlendMode() );
		P.setColor( mSceneNode->getHighlightInvalidationColor() );
		P.setLineWidth( PixelDensity::dpToPx( 2 ) );
		P.drawRectangle( getScreenBounds() );

		UIWidget::matrixUnset();
	}
}

void UIWindow::drawShadow() {
	if ( mStyleConfig.WinFlags & UI_WIN_SHADOW ) {
		UIWidget::matrixSet();

		Primitives P;
		P.setForceDraw( false );

		Color BeginC( 0, 0, 0, 25 * ( getAlpha() / (Float)255 ) );
		Color EndC( 0, 0, 0, 0 );
		Float SSize = PixelDensity::dpToPx( 16.f );

		Vector2f ShadowPos = mScreenPos + Vector2f( 0, SSize );

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

		UIWidget::matrixUnset();
	}
}

void UIWindow::onPaddingChange() {
	fixChildsSize();

	UIWidget::onPaddingChange();
}

Sizei UIWindow::getFrameBufferSize() {
	return isResizeable() && (Node*)this != mSceneNode ? Sizei( Math::nextPowOfTwo( (int)mSize.getWidth() ), Math::nextPowOfTwo( (int)mSize.getHeight() ) ) : mSize.ceil().asInt();
}

UISceneNode *UIWindow::getUISceneNode() {
	return ( NULL != mSceneNode && mSceneNode->isUISceneNode() ) ? static_cast<UISceneNode*>( mSceneNode ) : NULL;
}

void UIWindow::createModalControl() {
	Node * Ctrl = mSceneNode;

	if ( NULL == Ctrl )
		return;

	if ( NULL == mModalCtrl ) {
		mModalCtrl = UIWidget::New();
		mModalCtrl->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		mModalCtrl->setParent( Ctrl )->setPosition(0,0)->setSize( Ctrl->getSize() );
		mModalCtrl->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	} else {
		mModalCtrl->setPosition( 0, 0 );
		mModalCtrl->setSize( Ctrl->getSize() );
		mModalCtrl->updateAnchorsDistances();
	}

	mModalCtrl->setEnabled( false );
	mModalCtrl->setVisible( false );

	disableByModal();
}

void UIWindow::enableByModal() {
	if ( isModal() && NULL != mSceneNode ) {
		Node * CtrlChild = mSceneNode->getFirstChild();

		while ( NULL != CtrlChild )
		{
			if ( CtrlChild != mModalCtrl &&
				 CtrlChild != this &&
				 CtrlChild->getNodeFlags() & NODE_FLAG_DISABLED_BY_NODE )
			{
				CtrlChild->setEnabled( true );
				CtrlChild->writeCtrlFlag( NODE_FLAG_DISABLED_BY_NODE, 0 );
			}

			CtrlChild = CtrlChild->getNextNode();
		}
	}
}

void UIWindow::disableByModal() {
	if ( isModal() && NULL != mSceneNode ) {
		Node * CtrlChild = mSceneNode->getFirstChild();

		while ( NULL != CtrlChild )
		{
			if ( CtrlChild != mModalCtrl &&
				 CtrlChild != this &&
				 CtrlChild->isEnabled() )
			{
				CtrlChild->setEnabled( false );
				CtrlChild->writeCtrlFlag( NODE_FLAG_DISABLED_BY_NODE, 1 );
			}

			CtrlChild = CtrlChild->getNextNode();
		}
	}
}

Uint32 UIWindow::getType() const {
	return UI_TYPE_WINDOW;
}

bool UIWindow::isType( const Uint32& type ) const {
	return UIWindow::getType() == type ? true : UIWidget::isType( type );
}

void UIWindow::closeWindow() {
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
		runAction( Actions::Sequence::New( Actions::FadeOut::New( UIThemeManager::instance()->getControlsFadeOutTime() ), Actions::Close::New() ) );
	else
		close();
}

void UIWindow::close() {
	UIWidget::close();

	enableByModal();
}

void UIWindow::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	if ( NULL != mContainer )
		mContainer->setThemeSkin			( Theme, "winback"			);

	if ( !( mStyleConfig.WinFlags & UI_WIN_NO_BORDER ) ) {
		mWindowDecoration->setThemeSkin	( Theme, "windeco"			);
		mBorderLeft->setThemeSkin		( Theme, "winborderleft"	);
		mBorderRight->setThemeSkin		( Theme, "winborderright"	);
		mBorderBottom->setThemeSkin		( Theme, "winborderbottom"	);

		if ( NULL != mButtonClose ) {
			mButtonClose->setThemeSkin( Theme, "winclose" );
			mButtonClose->setSize( mButtonClose->getSkinSize() );
		}

		if ( NULL != mButtonMaximize ) {
			mButtonMaximize->setThemeSkin( Theme, "winmax" );
			mButtonMaximize->setSize( mButtonMaximize->getSkinSize() );
		}

		if ( NULL != mButtonMinimize ) {
			mButtonMinimize->setThemeSkin( Theme, "winmin" );
			mButtonMinimize->setSize( mButtonMinimize->getSkinSize() );
		}

		calcMinWinSize();
	}

	fixChildsSize();
}

void UIWindow::calcMinWinSize() {
	if ( NULL == mWindowDecoration || ( mStyleConfig.MinWindowSize.x != 0 && mStyleConfig.MinWindowSize.y != 0 ) )
		return;

	Sizei tSize;

	tSize.x = mBorderLeft->getSize().getWidth() + mBorderRight->getSize().getWidth() - mStyleConfig.ButtonsPositionFixer.x;
	tSize.y = mWindowDecoration->getSize().getHeight() + mBorderBottom->getSize().getHeight();

	if ( NULL != mButtonClose )
		tSize.x += mButtonClose->getSize().getWidth();

	if ( NULL != mButtonMaximize )
		tSize.x += mButtonMaximize->getSize().getWidth();

	if ( NULL != mButtonMinimize )
		tSize.x += mButtonMinimize->getSize().getWidth();

	if ( mStyleConfig.MinWindowSize.x < tSize.x )
		mStyleConfig.MinWindowSize.x = tSize.x;

	if ( mStyleConfig.MinWindowSize.y < tSize.y )
		mStyleConfig.MinWindowSize.y = tSize.y;
}

void UIWindow::applyMinWinSize() {
	if ( mDpSize.x < mStyleConfig.MinWindowSize.x && mDpSize.y < mStyleConfig.MinWindowSize.y ) {
		setSize( mStyleConfig.MinWindowSize );
	} else if ( mDpSize.x < mStyleConfig.MinWindowSize.x ) {
		setSize( Sizef( mStyleConfig.MinWindowSize.x, mDpSize.y ) );
	} else if ( mDpSize.y < mStyleConfig.MinWindowSize.y ) {
		setSize( Sizef( mDpSize.x, mStyleConfig.MinWindowSize.y ) );
	}
}

void UIWindow::onSizeChange() {
	if ( mDpSize.x < mStyleConfig.MinWindowSize.x || mDpSize.y < mStyleConfig.MinWindowSize.y ) {
		if ( mDpSize.x < mStyleConfig.MinWindowSize.x && mDpSize.y < mStyleConfig.MinWindowSize.y ) {
			setSize( mStyleConfig.MinWindowSize );
		} else if ( mDpSize.x < mStyleConfig.MinWindowSize.x ) {
			setSize( Sizef( mStyleConfig.MinWindowSize.x, mDpSize.y ) );
		} else {
			setSize( Sizef( mDpSize.x, mStyleConfig.MinWindowSize.y ) );
		}
	} else {
		fixChildsSize();

		if ( ownsFrameBuffer() && NULL != mFrameBuffer && ( mFrameBuffer->getWidth() < mSize.getWidth() || mFrameBuffer->getHeight() < mSize.getHeight() ) ) {
			if ( NULL == mFrameBuffer ) {
				createFrameBuffer();
			} else {
				Sizei fboSize( getFrameBufferSize() );
				mFrameBuffer->resize( fboSize.getWidth(), fboSize.getHeight() );
			}
		}

		UIWidget::onSizeChange();
	}
}

UINode * UIWindow::setSize( const Sizef& Size ) {
	if ( NULL != mWindowDecoration ) {
		Sizef size = Size;

		size.x += mBorderLeft->getSize().getWidth() + mBorderRight->getSize().getWidth();
		size.y += mWindowDecoration->getSize().getHeight() + mBorderBottom->getSize().getHeight();

		UIWidget::setSize( size );
	} else {
		UIWidget::setSize( Size );
	}

	return this;
}

UINode * UIWindow::setSize( const Float& Width, const Float& Height ) {
	setSize( Sizef( Width, Height ) );
	return this;
}

UIWindow *UIWindow::setSizeWithDecoration( const Float& Width, const Float& Height) {
	setSizeWithDecoration( Sizef( Width, Height ) );
	return this;
}

UIWindow *UIWindow::setSizeWithDecoration( const Sizef & size ) {
	UIWidget::setSize( size );
	return this;
}

const Sizef& UIWindow::getSize() {
	return UIWidget::getSize();
}

void UIWindow::fixChildsSize() {
	if ( mSize.getWidth() < PixelDensity::dpToPx( mStyleConfig.MinWindowSize.getWidth() ) || mSize.getHeight() < PixelDensity::dpToPx( mStyleConfig.MinWindowSize.getHeight() ) ) {
		internalSize( eemin( mSize.getWidth(), PixelDensity::dpToPx( mStyleConfig.MinWindowSize.getWidth() ) ), eemin( mSize.getHeight(), PixelDensity::dpToPx( mStyleConfig.MinWindowSize.getHeight() ) ) );
	}

	if ( NULL == mWindowDecoration && NULL != mContainer ) {
		mContainer->setPixelsSize( mSize - mRealPadding );
		mContainer->setPosition( mRealPadding.Left, mRealPadding.Top );
		return;
	}

	Sizei decoSize = mStyleConfig.DecorationSize;

	if ( mStyleConfig.DecorationAutoSize ) {
		decoSize = mStyleConfig.DecorationSize = Sizei( mDpSize.getWidth(), mWindowDecoration->getSkinSize().getHeight() );
	}

	mWindowDecoration->setPixelsSize( mSize.getWidth(), PixelDensity::dpToPx( mStyleConfig.DecorationSize.getHeight() ) );

	if ( mStyleConfig.BorderAutoSize ) {
		mBorderBottom->setPixelsSize( mSize.getWidth(), PixelDensity::dpToPx( mBorderBottom->getSkinSize().getHeight() ) );
	} else {
		mBorderBottom->setPixelsSize( mSize.getWidth(), PixelDensity::dpToPx( mStyleConfig.BorderSize.getHeight() ) );
	}

	Uint32 BorderHeight = mSize.getHeight() - PixelDensity::dpToPx( decoSize.getHeight() ) - mBorderBottom->getRealSize().getHeight();

	if ( mStyleConfig.BorderAutoSize ) {
		mBorderLeft->setPixelsSize( PixelDensity::dpToPx( mBorderLeft->getSkinSize().getWidth() ), BorderHeight );
		mBorderRight->setPixelsSize( PixelDensity::dpToPx( mBorderRight->getSkinSize().getWidth() ), BorderHeight );
	} else {
		mBorderLeft->setPixelsSize( PixelDensity::dpToPx( mStyleConfig.BorderSize.getWidth() ), BorderHeight );
		mBorderRight->setPixelsSize( PixelDensity::dpToPx( mStyleConfig.BorderSize.getWidth() ), BorderHeight );
	}

	mBorderLeft->setPixelsPosition( 0, mWindowDecoration->getRealSize().getHeight() );
	mBorderRight->setPixelsPosition( mSize.getWidth() - mBorderRight->getRealSize().getWidth(), mWindowDecoration->getRealSize().getHeight() );
	mBorderBottom->setPixelsPosition( 0, mWindowDecoration->getRealSize().getHeight() + mBorderLeft->getRealSize().getHeight() );

	mContainer->setPixelsPosition( mBorderLeft->getRealSize().getWidth() + mRealPadding.Left, mWindowDecoration->getRealSize().getHeight() + mRealPadding.Top );
	mContainer->setPixelsSize( mSize.getWidth() - mBorderLeft->getRealSize().getWidth() - mBorderRight->getRealSize().getWidth() - mRealPadding.Left - mRealPadding.Right,
							   mSize.getHeight() - mWindowDecoration->getRealSize().getHeight() - mBorderBottom->getRealSize().getHeight() - mRealPadding.Top - mRealPadding.Bottom );

	Uint32 yPos;
	Vector2f posFix( PixelDensity::dpToPx( Vector2f( mStyleConfig.ButtonsPositionFixer.x, mStyleConfig.ButtonsPositionFixer.y ) ) );

	if ( NULL != mButtonClose ) {
		yPos = mWindowDecoration->getRealSize().getHeight() / 2 - mButtonClose->getRealSize().getHeight() / 2 + posFix.y;

		mButtonClose->setPixelsPosition( mWindowDecoration->getRealSize().getWidth() - mBorderRight->getRealSize().getWidth() - mButtonClose->getRealSize().getWidth() + posFix.x, yPos );
	}

	if ( NULL != mButtonMaximize ) {
		yPos = mWindowDecoration->getRealSize().getHeight() / 2 - mButtonMaximize->getRealSize().getHeight() / 2 + posFix.y;

		if ( NULL != mButtonClose ) {
			mButtonMaximize->setPixelsPosition( mButtonClose->getRealPosition().x - PixelDensity::dpToPx( mStyleConfig.ButtonsSeparation ) - mButtonMaximize->getRealSize().getWidth(), yPos );
		} else {
			mButtonMaximize->setPixelsPosition( mWindowDecoration->getRealSize().getWidth() - mBorderRight->getRealSize().getWidth() - mButtonMaximize->getRealSize().getWidth() + posFix.x, yPos );
		}
	}

	if ( NULL != mButtonMinimize ) {
		yPos = mWindowDecoration->getRealSize().getHeight() / 2 - mButtonMinimize->getRealSize().getHeight() / 2 + posFix.y;

		if ( NULL != mButtonMaximize ) {
			mButtonMinimize->setPixelsPosition( mButtonMaximize->getRealPosition().x - PixelDensity::dpToPx( mStyleConfig.ButtonsSeparation ) - mButtonMinimize->getRealSize().getWidth(), yPos );
		} else {
			if ( NULL != mButtonClose ) {
				mButtonMinimize->setPixelsPosition( mButtonClose->getRealPosition().x - PixelDensity::dpToPx( mStyleConfig.ButtonsSeparation ) - mButtonMinimize->getRealSize().getWidth(), yPos );
			} else {
				mButtonMinimize->setPixelsPosition( mWindowDecoration->getRealSize().getWidth() - mBorderRight->getRealSize().getWidth() - mButtonMinimize->getRealSize().getWidth() + posFix.x, yPos );
			}
		}
	}

	fixTitleSize();
}

Uint32 UIWindow::onMessage( const NodeMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::Focus:
		{
			toFront();
			break;
		}
		case NodeMessage::MouseDown:
		{
			doResize( Msg );
			break;
		}
		case NodeMessage::WindowResize:
		{
			if ( isModal() && NULL != mModalCtrl && NULL != mSceneNode ) {
				mModalCtrl->setSize( mSceneNode->getSize() );
			}

			break;
		}
		case NodeMessage::MouseExit:
		{
			if ( getUISceneNode() != NULL )
				getUISceneNode()->setCursor( EE_CURSOR_ARROW );
			break;
		}
		case NodeMessage::DragStart:
		{
			if ( getUISceneNode() != NULL )
				getUISceneNode()->setCursor( EE_CURSOR_HAND );

			toFront();

			break;
		}
		case NodeMessage::DragStop:
		{
			if ( getUISceneNode() != NULL )
				getUISceneNode()->setCursor( EE_CURSOR_ARROW );
			break;
		}
		case NodeMessage::Click:
		{
			if ( ( mStyleConfig.WinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) && ( Msg->getFlags() & EE_BUTTON_LMASK ) ) {
				if ( Msg->getSender() == mButtonClose ) {
					closeWindow();

					sendCommonEvent( Event::OnWindowCloseClick );
				} else if ( Msg->getSender() == mButtonMaximize ) {
					maximize();

					sendCommonEvent( Event::OnWindowMaximizeClick );
				} else if ( Msg->getSender() == mButtonMinimize ) {
					hide();

					sendCommonEvent( Event::OnWindowMinimizeClick );
				}
			}

			break;
		}
	}

	return UIWidget::onMessage( Msg );
}

void UIWindow::doResize ( const NodeMessage * Msg ) {
	if ( NULL == mWindowDecoration || NULL == getEventDispatcher() )
		return;

	if (	!isResizeable() ||
			!( Msg->getFlags() & EE_BUTTON_LMASK ) ||
			RESIZE_NONE != mResizeType ||
			( getEventDispatcher()->getLastPressTrigger() & EE_BUTTON_LMASK )
	)
		return;

	decideResizeType( Msg->getSender() );
}

void UIWindow::decideResizeType( Node * Control ) {
	if ( NULL == getEventDispatcher() )
		return;

	Vector2i Pos = getEventDispatcher()->getMousePos();

	worldToNode( Pos );

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->getSize().getWidth() ) {
			tryResize( RESIZE_TOPLEFT );
		} else if ( Pos.x >= ( mDpSize.getWidth() - mBorderRight->getSize().getWidth() ) ) {
			tryResize( RESIZE_TOPRIGHT );
		} else if ( Pos.y <= mBorderBottom->getSize().getHeight() ) {
			if ( Pos.x < mStyleConfig.MinCornerDistance ) {
				tryResize( RESIZE_TOPLEFT );
			} else if ( Pos.x > mDpSize.getWidth() - mStyleConfig.MinCornerDistance ) {
				tryResize( RESIZE_TOPRIGHT );
			} else {
				tryResize( RESIZE_TOP );
			}
		}
	} else if ( Control == mBorderBottom ) {
		if ( Pos.x < mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_LEFTBOTTOM );
		} else if ( Pos.x > mDpSize.getWidth() - mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_RIGHTBOTTOM );
		} else {
			tryResize( RESIZE_BOTTOM );
		}
	} else if ( Control == mBorderLeft )  {
		if ( Pos.y >= mDpSize.getHeight() - mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_LEFTBOTTOM );
		} else {
			tryResize( RESIZE_LEFT );
		}
	} else if ( Control == mBorderRight ) {
		if ( Pos.y >= mDpSize.getHeight() - mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_RIGHTBOTTOM );
		} else {
			tryResize( RESIZE_RIGHT );
		}
	}
}

void UIWindow::tryResize( const UI_RESIZE_TYPE& Type ) {
	if ( RESIZE_NONE != mResizeType || NULL == getEventDispatcher() )
		return;

	setDragEnabled( false );

	Vector2f Pos = getEventDispatcher()->getMousePosf();

	worldToNode( Pos );
	
	mResizeType = Type;

	Pos = PixelDensity::dpToPx( Pos );

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
	if ( RESIZE_NONE == mResizeType || NULL == getEventDispatcher() )
		return;

	if ( !( getEventDispatcher()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
		endResize();
		setDragEnabled( true );
		return;
	}

	Vector2f Pos = getEventDispatcher()->getMousePosf();

	worldToNode( Pos );

	Pos = PixelDensity::dpToPx( Pos );

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
			UINode::setPixelsPosition( mPosition.x + Pos.x, mPosition.y );
			internalSize( mSize.getWidth() - Pos.x, mSize.getHeight() );
			break;
		}
		case RESIZE_TOP:
		{
			Pos.y -= mResizePos.y;
			UINode::setPixelsPosition( mPosition.x, mPosition.y + Pos.y );
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
			UINode::setPixelsPosition( mPosition.x + Pos.x, mPosition.y + Pos.y );
			internalSize( mSize.getWidth() - Pos.x, mSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_TOPRIGHT:
		{
			Pos.y -= mResizePos.y;
			Pos.x += mResizePos.x;
			UINode::setPixelsPosition( mPosition.x, mPosition.y + Pos.y );
			internalSize( Pos.x, mSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_LEFTBOTTOM:
		{
			Pos.x -= mResizePos.x;
			Pos.y += mResizePos.y;
			UINode::setPixelsPosition( mPosition.x + Pos.x, mPosition.y );
			internalSize( mSize.getWidth() - Pos.x, Pos.y );
			break;
		}
		case RESIZE_NONE:
		{
		}
	}
}

void UIWindow::internalSize(const Float & w, const Float & h ) {
	internalSize( Sizef( w, h ) );
}

void UIWindow::internalSize( Sizef Size ) {
	Sizef realMin = PixelDensity::dpToPx( mStyleConfig.MinWindowSize );

	Size.x = eemax( realMin.x, Size.x );
	Size.y = eemax( realMin.y, Size.y );

	if ( Size != mSize ) {
		setInternalPixelsSize( Size );
		onSizeChange();
	}
}

void UIWindow::update( const Time& time ) {
	resizeCursor();

	UIWidget::update( time );

	updateResize();
}

UIWidget * UIWindow::getContainer() const {
	return mContainer;
}

UINode * UIWindow::getButtonClose() const {
	return mButtonClose;
}

UINode * UIWindow::getButtonMaximize() const {
	return mButtonMaximize;
}

UINode * UIWindow::getButtonMinimize() const {
	return mButtonMinimize;
}

bool UIWindow::show() {
	if ( !isVisible() ) {
		setEnabled( true );
		setVisible( true );

		setFocus();

		runAction( Actions::Fade::New( mStyleConfig.BaseAlpha == getAlpha() ? 0.f : mAlpha, mStyleConfig.BaseAlpha, UIThemeManager::instance()->getControlsFadeOutTime() ) );

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

bool UIWindow::hide() {
	if ( isVisible() ) {
		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			runAction( Actions::Sequence::New( Actions::FadeOut::New( UIThemeManager::instance()->getControlsFadeOutTime() ), Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ) ) ) );
		} else {
			setEnabled( false );
			setVisible( false );
		}

		if ( NULL != mSceneNode )
			mSceneNode->setFocus();

		if ( NULL != mModalCtrl ) {
			mModalCtrl->setEnabled( false );
			mModalCtrl->setVisible( false );
		}

		return true;
	}

	return false;
}

void UIWindow::onAlphaChange() {
	if ( mStyleConfig.WinFlags & UI_WIN_SHARE_ALPHA_WITH_CHILDS ) {
		Node * CurChild = mChild;

		while ( NULL != CurChild ) {
			CurChild->setAlpha( mAlpha );
			CurChild = CurChild->getNextNode();
		}
	}

	UIWidget::onAlphaChange();
}

void UIWindow::onChildCountChange() {
	if ( NULL == mContainer )
		return;

	Node * child = mChild;
	bool found = false;

	while ( NULL != child ) {
		if ( !( child->getNodeFlags() & NODE_FLAG_OWNED_BY_NODE ) ) {
			found = true;
			break;
		}

		child = child->getNextNode();
	}

	if ( found ) {
		child->setParent( mContainer );
	}
}

void UIWindow::setBaseAlpha( const Uint8& Alpha ) {
	if ( mAlpha == mStyleConfig.BaseAlpha ) {
		UINode::setAlpha( Alpha );
	}

	mStyleConfig.BaseAlpha = Alpha;
}

const Uint8& UIWindow::getBaseAlpha() const {
	return mStyleConfig.BaseAlpha;
}

void UIWindow::setTitle( const String& text ) {
	if ( NULL == mTitle ) {
		mTitle = UITextView::New();
		mTitle->setLayoutSizeRules( FIXED, FIXED );
		mTitle->writeCtrlFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		mTitle->setParent( this );
		mTitle->setHorizontalAlign( getHorizontalAlign() );
		mTitle->setVerticalAlign( getVerticalAlign() );
		mTitle->setFontColor( mStyleConfig.TitleFontColor );

		if ( mStyleConfig.Style & Text::Shadow ) {
			UIFontStyleConfig fsc = mTitle->getFontStyleConfig();
			fsc.Style |= Text::Shadow;
			mTitle->setFontStyleConfig( fsc );
		}

		mTitle->setEnabled( false );
		mTitle->setVisible( !( mStyleConfig.WinFlags & UI_WIN_NO_BORDER ) );
	}

	fixTitleSize();

	mTitle->setText( text );
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

UITextView * UIWindow::getTitleTextBox() const {
	return mTitle;
}

void UIWindow::maximize() {
	Node * Ctrl = mSceneNode;

	if ( NULL == Ctrl )
		return;

	if ( Ctrl->getSize() == mDpSize ) {
		setPixelsPosition( mNonMaxPos );
		internalSize( mNonMaxSize );
	} else {
		mNonMaxPos	= mPosition;
		mNonMaxSize = mSize;

		setPosition( 0, 0 );
		internalSize( Ctrl->getRealSize() );
	}
}

Uint32 UIWindow::onMouseDoubleClick( const Vector2i &Pos, const Uint32 Flags ) {
	if ( isResizeable() && ( NULL != mButtonMaximize ) && ( Flags & EE_BUTTON_LMASK ) ) {
		maximize();

		sendCommonEvent( Event::OnWindowMaximizeClick );
	}

	return 1;
}

Uint32 UIWindow::onKeyDown( const KeyEvent &Event ) {
	checkShortcuts( Event.getKeyCode(), Event.getMod() );

	return UIWidget::onKeyDown( Event );
}

void UIWindow::internalDraw() {
	if ( mVisible && 0 != mAlpha ) {
		updateScreenPos();

		preDraw();

		drawShadow();

		ClippingMask * clippingMask = GLi->getClippingMask();

		std::list<Rectf> clips = clippingMask->getPlanesClipped();

		if ( !clips.empty() )
			clippingMask->clipPlaneDisable();

		matrixSet();

		if ( !ownsFrameBuffer() || ( NULL != mSceneNode && !mSceneNode->usesInvalidation() ) || invalidated() ) {
			clipStart();

			draw();

			drawChilds();

			clipEnd();
		}

		matrixUnset();

		if ( !clips.empty() )
			clippingMask->setPlanesClipped( clips );

		postDraw();

		drawHighlightInvalidation();

		writeCtrlFlag( NODE_FLAG_VIEW_DIRTY, 0 );
	}
}

void UIWindow::invalidate() {
	if ( mVisible && mAlpha != 0.f ) {
		writeCtrlFlag( NODE_FLAG_VIEW_DIRTY, 1 );

		if ( NULL != mSceneNode )
			mSceneNode->invalidate();
	}
}

FrameBuffer * UIWindow::getFrameBuffer() const {
	return mFrameBuffer;
}

bool UIWindow::isDrawInvalidator() {
	return NULL != mFrameBuffer;
}

bool UIWindow::invalidated() {
	return 0 != ( mNodeFlags & NODE_FLAG_VIEW_DIRTY );
}

void UIWindow::matrixSet() {
	if ( ownsFrameBuffer() ) {
		if ( NULL != mFrameBuffer ) {
			if ( ( NULL != mSceneNode && !mSceneNode->usesInvalidation() ) || invalidated() ) {
				mFrameBufferBound = true;

				mFrameBuffer->bind();

				mFrameBuffer->clear();
			}

			if ( 0 != mScreenPosi ) {
				GLi->pushMatrix();
				GLi->translatef( -mScreenPosi.x , -mScreenPosi.y, 0.f );
			}
		}
	} else {
		UIWidget::matrixSet();
	}
}

void UIWindow::matrixUnset() {
	if ( ownsFrameBuffer() ) {
		GlobalBatchRenderer::instance()->draw();

		if ( 0 != mScreenPosi )
			GLi->popMatrix();

		if ( mFrameBufferBound ) {
			mFrameBuffer->unbind();

			mFrameBufferBound = false;
		}

		drawFrameBuffer();
	} else {
		UIWidget::matrixUnset();
	}
}

bool UIWindow::ownsFrameBuffer() {
	return 0 != ( mStyleConfig.WinFlags & UI_WIN_FRAME_BUFFER );
}

void UIWindow::checkShortcuts( const Uint32& KeyCode, const Uint32& Mod ) {
	if ( NULL == getEventDispatcher() )
		return;

	for ( KeyboardShortcuts::iterator it = mKbShortcuts.begin(); it != mKbShortcuts.end(); ++it ) {
		KeyboardShortcut kb = (*it);

		if ( KeyCode == kb.KeyCode && ( Mod & kb.Mod ) ) {
			getEventDispatcher()->sendMouseUp( kb.Button, Vector2i(0,0), EE_BUTTON_LMASK );
			getEventDispatcher()->sendMouseClick( kb.Button, Vector2i(0,0), EE_BUTTON_LMASK );
		}
	}
}

UIWindow::KeyboardShortcuts::iterator UIWindow::existsShortcut( const Uint32& KeyCode, const Uint32& Mod ) {
	for ( KeyboardShortcuts::iterator it = mKbShortcuts.begin(); it != mKbShortcuts.end(); ++it ) {
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
	return isResizeable() && 0 != ( mStyleConfig.WinFlags & UI_WIN_MAXIMIZE_BUTTON );
}

bool UIWindow::isResizeable() {
	return 0 != ( mStyleConfig.WinFlags & UI_WIN_RESIZEABLE );
}

Uint32 UIWindow::getWinFlags() const {
	return mStyleConfig.WinFlags;
}

UIWindow * UIWindow::setWinFlags(const Uint32 & winFlags) {
	mStyleConfig.WinFlags = winFlags;

	updateWinFlags();

	return this;
}

UIWindowStyleConfig UIWindow::getStyleConfig() const {
	return mStyleConfig;
}

UIWindow * UIWindow::setStyleConfig(const UIWindowStyleConfig & styleConfig) {
	mStyleConfig = styleConfig;

	updateWinFlags();

	setAlpha( mStyleConfig.BaseAlpha );

	applyDefaultTheme();

	applyMinWinSize();

	return this;
}

UIWindow * UIWindow::setMinWindowSize( const Float& width, const Float& height ) {
	return setMinWindowSize( Sizef( width, height ) );
}

UIWindow * UIWindow::setMinWindowSize( Sizef size ) {
	mStyleConfig.MinWindowSize = size;

	applyMinWinSize();

	return this;
}

const Sizef& UIWindow::getMinWindowSize() {
	return mStyleConfig.MinWindowSize;
}

bool UIWindow::isModal() {
	return 0 != ( mStyleConfig.WinFlags & UI_WIN_MODAL );
}

UIWidget * UIWindow::getModalControl() const {
	return mModalCtrl;
}

void UIWindow::resizeCursor() {
	UISceneNode * sceneNode = getUISceneNode();

	if ( NULL == sceneNode || !isMouseOverMeOrChilds() || !sceneNode->getUseGlobalCursors() || ( mStyleConfig.WinFlags & UI_WIN_NO_BORDER ) || !isResizeable() )
		return;

	EventDispatcher * eventDispatcher = sceneNode->getEventDispatcher();

	Vector2i Pos = eventDispatcher->getMousePos();

	worldToNode( Pos );

	const Node * Control = eventDispatcher->getOverControl();

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->getSize().getWidth() ) {
			sceneNode->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_TOPLEFT
		} else if ( Pos.x >= ( mDpSize.getWidth() - mBorderRight->getSize().getWidth() ) ) {
			sceneNode->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_TOPRIGHT
		} else if ( Pos.y <= mBorderBottom->getSize().getHeight() ) {
			if ( Pos.x < mStyleConfig.MinCornerDistance ) {
				sceneNode->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_TOPLEFT
			} else if ( Pos.x > mDpSize.getWidth() - mStyleConfig.MinCornerDistance ) {
				sceneNode->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_TOPRIGHT
			} else {
				sceneNode->setCursor( EE_CURSOR_SIZENS ); // RESIZE_TOP
			}
		} else if ( !( eventDispatcher->getPressTrigger() & EE_BUTTON_LMASK ) ) {
			sceneNode->setCursor( EE_CURSOR_ARROW );
		}
	} else if ( Control == mBorderBottom ) {
		if ( Pos.x < mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_LEFTBOTTOM
		} else if ( Pos.x > mDpSize.getWidth() - mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			sceneNode->setCursor( EE_CURSOR_SIZENS ); // RESIZE_BOTTOM
		}
	} else if ( Control == mBorderLeft )  {
		if ( Pos.y >= mDpSize.getHeight() - mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( EE_CURSOR_SIZENESW ); // RESIZE_LEFTBOTTOM
		} else {
			sceneNode->setCursor( EE_CURSOR_SIZEWE ); // RESIZE_LEFT
		}
	} else if ( Control == mBorderRight ) {
		if ( Pos.y >= mDpSize.getHeight() - mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( EE_CURSOR_SIZENWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			sceneNode->setCursor( EE_CURSOR_SIZEWE ); // RESIZE_RIGHT
		}
	}
}

bool UIWindow::setAttribute( const NodeAttribute& attribute ) {
	const std::string& name = attribute.getName();

	if ( "width" == name ) {
		setSize( attribute.asInt(), mDpSize.getHeight() );
	} else if ( "height" == name ) {
		setSize( mDpSize.getWidth(), attribute.asInt() );
	} else if ( "title" == name ) {
		setTitle( attribute.asString() );
	} else if ( "basealpha" == name ) {
		unsigned int val = attribute.asUint();
		if ( val <= 255 )
			setBaseAlpha( (Uint8)val );
	} else if ( "winflags" == name ) {
		std::string flagsStr = attribute.asString();
		String::toLowerInPlace( flagsStr );
		std::vector<std::string> strings = String::split( flagsStr, '|' );
		Uint32 winflags = 0;

		if ( strings.size() ) {
			for ( std::size_t i = 0; i < strings.size(); i++ ) {
				std::string cur = strings[i];

				if ( "default" == cur ) winflags |= UI_WIN_DEFAULT_FLAGS;
				else if ( "close" == cur ) winflags |= UI_WIN_CLOSE_BUTTON;
				else if ( "maximize" == cur ) winflags |= UI_WIN_MAXIMIZE_BUTTON;
				else if ( "minimize" == cur ) winflags |= UI_WIN_MINIMIZE_BUTTON;
				else if ( "dragable" == cur ) winflags |= UI_WIN_DRAGABLE_CONTAINER;
				else if ( "shadow" == cur ) winflags |= UI_WIN_SHADOW;
				else if ( "modal" == cur ) winflags |= UI_WIN_MODAL;
				else if ( "noborder" == cur || "borderless" == cur ) winflags |= UI_WIN_NO_BORDER;
				else if ( "resizeable" == cur ) winflags |= UI_WIN_RESIZEABLE;
				else if ( "sharealpha" == cur ) winflags |= UI_WIN_SHARE_ALPHA_WITH_CHILDS;
				else if ( "buttonactions" == cur ) winflags |= UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS;
				else if ( "framebuffer"== cur ) winflags |= UI_WIN_FRAME_BUFFER;
				else if ( "colorbuffer"== cur ) winflags |= UI_WIN_COLOR_BUFFER;
			}

			setWinFlags( winflags );
		}
	} else {
		return UIWidget::setAttribute( attribute );
	}

	return true;
}

void UIWindow::loadFromXmlNode(const pugi::xml_node & node) {
	UIWidget::loadFromXmlNode( node );

	show();
}

void UIWindow::preDraw() {
}

void UIWindow::postDraw() {
}

}}
