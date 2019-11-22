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

UIWindow * UIWindow::NewOpt( UIWindow::WindowBaseContainerType type, const StyleConfig& windowStyleConfig ) {
	return eeNew( UIWindow, ( type, windowStyleConfig ) );
}

UIWindow * UIWindow::New() {
	return eeNew( UIWindow, ( SIMPLE_LAYOUT ) );
}

UIWindow::UIWindow( UIWindow::WindowBaseContainerType type ) :
	UIWindow( type, StyleConfig() )
{}

UIWindow::UIWindow( UIWindow::WindowBaseContainerType type, const StyleConfig& windowStyleConfig ) :
	UIWidget( "window" ),
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
	subscribeScheduledUpdate();

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
	mContainer->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
	mContainer->setParent( this );
	mContainer->clipEnable();
	mContainer->enableReportSizeChangeToChilds();
	mContainer->setSize( getSize() );
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

void UIWindow::onContainerPositionChange( const Event * ) {
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

	writeNodeFlag( NODE_FLAG_FRAME_BUFFER, ( mStyleConfig.WinFlags & UI_WIN_FRAME_BUFFER ) ? 1 : 0 );

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

	if ( !( mStyleConfig.WinFlags & UI_WIN_NO_DECORATION ) ) {
		if ( NULL == mWindowDecoration ) {
			mWindowDecoration = UINode::New();
			mWindowDecoration->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mWindowDecoration->setParent( this );
		mWindowDecoration->setVisible( true );
		mWindowDecoration->setEnabled( false );

		if ( NULL == mBorderLeft ) {
			mBorderLeft		= UINode::New();
			mBorderLeft->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mBorderLeft->setParent( this );
		mBorderLeft->setEnabled( true );
		mBorderLeft->setVisible( true );

		if ( NULL == mBorderRight ) {
			mBorderRight	= UINode::New();
			mBorderRight->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mBorderRight->setParent( this );
		mBorderRight->setEnabled( true );
		mBorderRight->setVisible( true );

		if ( NULL == mBorderBottom ) {
			mBorderBottom	= UINode::New();
			mBorderBottom->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mBorderBottom->setParent( this );
		mBorderBottom->setEnabled( true );
		mBorderBottom->setVisible( true );

		if ( mStyleConfig.WinFlags & UI_WIN_CLOSE_BUTTON ) {
			if ( NULL == mButtonClose ) {
				mButtonClose = UINode::New();
				mButtonClose->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
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
				mButtonMaximize->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
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
				mButtonMinimize->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
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
			TextureRegion textureRegion( mFrameBuffer->getTexture()->getTextureId(), Rect( 0, 0, mSize.getWidth(), mSize.getHeight() ) );
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
		mModalCtrl = UIWidget::NewWithTag( "window::modaldialog" );
		mModalCtrl->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		mModalCtrl->setParent( Ctrl )->setPosition(0,0)->setSize( Ctrl->getSize() );
		mModalCtrl->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	} else {
		mModalCtrl->setPosition( 0, 0 );
		mModalCtrl->setSize( Ctrl->getSize() );
		mModalCtrl->updateAnchorsDistances();
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

	if ( Time::Zero != UIThemeManager::instance()->getControlsFadeOutTime() )
		runAction( Actions::Sequence::New( Actions::FadeOut::New( UIThemeManager::instance()->getControlsFadeOutTime() ), Actions::Close::New() ) );
	else
		close();
}

void UIWindow::close() {
	UIWidget::close();

	if ( NULL != mModalCtrl ) {
		mModalCtrl->setEnabled( false );
		mModalCtrl->setVisible( false );
		mModalCtrl->close();
		mModalCtrl = NULL;
	}
}

void UIWindow::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	if ( NULL != mContainer )
		mContainer->setThemeSkin			( Theme, "winback"			);

	if ( !( mStyleConfig.WinFlags & UI_WIN_NO_DECORATION ) ) {
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
	onThemeLoaded();
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
	if ( getSize().getWidth() < mStyleConfig.MinWindowSize.getWidth() && getSize().getHeight() < mStyleConfig.MinWindowSize.getHeight() ) {
		setSize( mStyleConfig.MinWindowSize );
	} else if ( getSize().getWidth() < mStyleConfig.MinWindowSize.getWidth() ) {
		setSize( Sizef( mStyleConfig.MinWindowSize.getWidth(), getSize().getHeight() ) );
	} else if ( getSize().getHeight() < mStyleConfig.MinWindowSize.getHeight() ) {
		setSize( Sizef( getSize().getWidth(), mStyleConfig.MinWindowSize.getHeight() ) );
	}
}

void UIWindow::onSizeChange() {
	if ( getSize().getWidth() < mStyleConfig.MinWindowSize.getWidth() || getSize().getHeight() < mStyleConfig.MinWindowSize.getHeight() ) {
		if ( getSize().getWidth() < mStyleConfig.MinWindowSize.getWidth() && getSize().getHeight() < mStyleConfig.MinWindowSize.getHeight() ) {
			setSize( mStyleConfig.MinWindowSize );
		} else if ( getSize().getWidth() < mStyleConfig.MinWindowSize.getHeight() ) {
			setSize( Sizef( mStyleConfig.MinWindowSize.getWidth(), getSize().getHeight() ) );
		} else {
			setSize( Sizef( getSize().getWidth(), mStyleConfig.MinWindowSize.getHeight() ) );
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

const Sizef& UIWindow::getSize() const {
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
		decoSize = mStyleConfig.DecorationSize = Sizei( getSize().getWidth(), mWindowDecoration->getSkinSize().getHeight() );
	}

	mWindowDecoration->setPixelsSize( mSize.getWidth(), PixelDensity::dpToPx( mStyleConfig.DecorationSize.getHeight() ) );

	if ( mStyleConfig.BorderAutoSize ) {
		mBorderBottom->setPixelsSize( mSize.getWidth(), PixelDensity::dpToPx( mBorderBottom->getSkinSize().getHeight() ) );
	} else {
		mBorderBottom->setPixelsSize( mSize.getWidth(), PixelDensity::dpToPx( mStyleConfig.BorderSize.getHeight() ) );
	}

	Uint32 BorderHeight = mSize.getHeight() - PixelDensity::dpToPx( decoSize.getHeight() ) - mBorderBottom->getPixelsSize().getHeight();

	if ( mStyleConfig.BorderAutoSize ) {
		mBorderLeft->setPixelsSize( PixelDensity::dpToPx( mBorderLeft->getSkinSize().getWidth() ), BorderHeight );
		mBorderRight->setPixelsSize( PixelDensity::dpToPx( mBorderRight->getSkinSize().getWidth() ), BorderHeight );
	} else {
		mBorderLeft->setPixelsSize( PixelDensity::dpToPx( mStyleConfig.BorderSize.getWidth() ), BorderHeight );
		mBorderRight->setPixelsSize( PixelDensity::dpToPx( mStyleConfig.BorderSize.getWidth() ), BorderHeight );
	}

	mBorderLeft->setPixelsPosition( 0, mWindowDecoration->getPixelsSize().getHeight() );
	mBorderRight->setPixelsPosition( mSize.getWidth() - mBorderRight->getPixelsSize().getWidth(), mWindowDecoration->getPixelsSize().getHeight() );
	mBorderBottom->setPixelsPosition( 0, mWindowDecoration->getPixelsSize().getHeight() + mBorderLeft->getPixelsSize().getHeight() );

	mContainer->setPixelsPosition( mBorderLeft->getPixelsSize().getWidth() + mRealPadding.Left, mWindowDecoration->getPixelsSize().getHeight() + mRealPadding.Top );
	mContainer->setPixelsSize( mSize.getWidth() - mBorderLeft->getPixelsSize().getWidth() - mBorderRight->getPixelsSize().getWidth() - mRealPadding.Left - mRealPadding.Right,
							   mSize.getHeight() - mWindowDecoration->getPixelsSize().getHeight() - mBorderBottom->getPixelsSize().getHeight() - mRealPadding.Top - mRealPadding.Bottom );

	Uint32 yPos;
	Vector2f posFix( PixelDensity::dpToPx( Vector2f( mStyleConfig.ButtonsPositionFixer.x, mStyleConfig.ButtonsPositionFixer.y ) ) );

	if ( NULL != mButtonClose ) {
		yPos = mWindowDecoration->getPixelsSize().getHeight() / 2 - mButtonClose->getPixelsSize().getHeight() / 2 + posFix.y;

		mButtonClose->setPixelsPosition( mWindowDecoration->getPixelsSize().getWidth() - mBorderRight->getPixelsSize().getWidth() - mButtonClose->getPixelsSize().getWidth() + posFix.x, yPos );
	}

	if ( NULL != mButtonMaximize ) {
		yPos = mWindowDecoration->getPixelsSize().getHeight() / 2 - mButtonMaximize->getPixelsSize().getHeight() / 2 + posFix.y;

		if ( NULL != mButtonClose ) {
			mButtonMaximize->setPixelsPosition( mButtonClose->getPixelsPosition().x - PixelDensity::dpToPx( mStyleConfig.ButtonsSeparation ) - mButtonMaximize->getPixelsSize().getWidth(), yPos );
		} else {
			mButtonMaximize->setPixelsPosition( mWindowDecoration->getPixelsSize().getWidth() - mBorderRight->getPixelsSize().getWidth() - mButtonMaximize->getPixelsSize().getWidth() + posFix.x, yPos );
		}
	}

	if ( NULL != mButtonMinimize ) {
		yPos = mWindowDecoration->getPixelsSize().getHeight() / 2 - mButtonMinimize->getPixelsSize().getHeight() / 2 + posFix.y;

		if ( NULL != mButtonMaximize ) {
			mButtonMinimize->setPixelsPosition( mButtonMaximize->getPixelsPosition().x - PixelDensity::dpToPx( mStyleConfig.ButtonsSeparation ) - mButtonMinimize->getPixelsSize().getWidth(), yPos );
		} else {
			if ( NULL != mButtonClose ) {
				mButtonMinimize->setPixelsPosition( mButtonClose->getPixelsPosition().x - PixelDensity::dpToPx( mStyleConfig.ButtonsSeparation ) - mButtonMinimize->getPixelsSize().getWidth(), yPos );
			} else {
				mButtonMinimize->setPixelsPosition( mWindowDecoration->getPixelsSize().getWidth() - mBorderRight->getPixelsSize().getWidth() - mButtonMinimize->getPixelsSize().getWidth() + posFix.x, yPos );
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
		case NodeMessage::MouseLeave:
		{
			if ( getUISceneNode() != NULL )
				getUISceneNode()->setCursor( Cursor::Arrow );
			break;
		}
		case NodeMessage::DragStart:
		{
			if ( getUISceneNode() != NULL )
				getUISceneNode()->setCursor( Cursor::Hand );

			toFront();

			break;
		}
		case NodeMessage::DragStop:
		{
			if ( getUISceneNode() != NULL )
				getUISceneNode()->setCursor( Cursor::Arrow );
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
		} else if ( Pos.x >= ( getSize().getWidth() - mBorderRight->getSize().getWidth() ) ) {
			tryResize( RESIZE_TOPRIGHT );
		} else if ( Pos.y <= mBorderBottom->getSize().getHeight() ) {
			if ( Pos.x < mStyleConfig.MinCornerDistance ) {
				tryResize( RESIZE_TOPLEFT );
			} else if ( Pos.x > getSize().getWidth() - mStyleConfig.MinCornerDistance ) {
				tryResize( RESIZE_TOPRIGHT );
			} else {
				tryResize( RESIZE_TOP );
			}
		}
	} else if ( Control == mBorderBottom ) {
		if ( Pos.x < mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_LEFTBOTTOM );
		} else if ( Pos.x > getSize().getWidth() - mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_RIGHTBOTTOM );
		} else {
			tryResize( RESIZE_BOTTOM );
		}
	} else if ( Control == mBorderLeft )  {
		if ( Pos.y >= getSize().getHeight() - mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_LEFTBOTTOM );
		} else {
			tryResize( RESIZE_LEFT );
		}
	} else if ( Control == mBorderRight ) {
		if ( Pos.y >= getSize().getHeight() - mStyleConfig.MinCornerDistance ) {
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

void UIWindow::scheduledUpdate( const Time& ) {
	resizeCursor();

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

void UIWindow::setupModal() {
	if ( isModal() ) {
		createModalControl();

		mModalCtrl->setEnabled( true );
		mModalCtrl->setVisible( true );
		mModalCtrl->toFront();

		toFront();
	}
}

bool UIWindow::show() {
	if ( !isVisible() ) {
		setEnabled( true );
		setVisible( true );

		setFocus();

		runAction( Actions::Fade::New( mStyleConfig.BaseAlpha == getAlpha() ? 0.f : mAlpha, mStyleConfig.BaseAlpha, UIThemeManager::instance()->getControlsFadeOutTime() ) );

		setupModal();

		return true;
	}

	setupModal();

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

void UIWindow::onPositionChange() {
	// Invalidate the buffer since a position change can get childs into a drawable position
	// (on screen), when the drawable could have been outside the viewport and not drawn in the
	// previous position.
	invalidate();

	UIWidget::onPositionChange();
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
		mTitle = UITextView::NewWithTag( "window::title" );
		mTitle->setLayoutSizeRules( FIXED, FIXED );
		mTitle->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		mTitle->setParent( this );
		mTitle->setHorizontalAlign( getHorizontalAlign() );
		mTitle->setVerticalAlign( getVerticalAlign() );
		mTitle->setEnabled( false );
		mTitle->setVisible( !( mStyleConfig.WinFlags & UI_WIN_NO_DECORATION ) );
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

	if ( Ctrl->getSize() == getSize() ) {
		setPixelsPosition( mNonMaxPos );
		internalSize( mNonMaxSize );
	} else {
		mNonMaxPos	= mPosition;
		mNonMaxSize = mSize;

		setPosition( 0, 0 );
		internalSize( Ctrl->getPixelsSize() );
	}
}

Uint32 UIWindow::onMouseDoubleClick( const Vector2i &, const Uint32 & Flags ) {
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

		writeNodeFlag( NODE_FLAG_VIEW_DIRTY, 0 );
	}
}

void UIWindow::invalidate() {
	if ( mVisible && mAlpha != 0.f ) {
		writeNodeFlag( NODE_FLAG_VIEW_DIRTY, 1 );

		if ( NULL != mSceneNode )
			mSceneNode->invalidate();
	}
}

FrameBuffer * UIWindow::getFrameBuffer() const {
	return mFrameBuffer;
}

bool UIWindow::isDrawInvalidator() const {
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

const UIWindow::StyleConfig& UIWindow::getStyleConfig() const {
	return mStyleConfig;
}

UIWindow * UIWindow::setStyleConfig(const StyleConfig & styleConfig) {
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

	if ( NULL == sceneNode || !isMouseOverMeOrChilds() || !sceneNode->getUseGlobalCursors() || ( mStyleConfig.WinFlags & UI_WIN_NO_DECORATION ) || !isResizeable() )
		return;

	EventDispatcher * eventDispatcher = sceneNode->getEventDispatcher();

	Vector2i Pos = eventDispatcher->getMousePos();

	worldToNode( Pos );

	const Node * Control = eventDispatcher->getOverControl();

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->getSize().getWidth() ) {
			sceneNode->setCursor( Cursor::SizeNWSE ); // RESIZE_TOPLEFT
		} else if ( Pos.x >= ( getSize().getWidth() - mBorderRight->getSize().getWidth() ) ) {
			sceneNode->setCursor( Cursor::SizeNESW ); // RESIZE_TOPRIGHT
		} else if ( Pos.y <= mBorderBottom->getSize().getHeight() ) {
			if ( Pos.x < mStyleConfig.MinCornerDistance ) {
				sceneNode->setCursor( Cursor::SizeNWSE ); // RESIZE_TOPLEFT
			} else if ( Pos.x > getSize().getWidth() - mStyleConfig.MinCornerDistance ) {
				sceneNode->setCursor( Cursor::SizeNESW ); // RESIZE_TOPRIGHT
			} else {
				sceneNode->setCursor( Cursor::SizeNS ); // RESIZE_TOP
			}
		} else if ( !( eventDispatcher->getPressTrigger() & EE_BUTTON_LMASK ) ) {
			sceneNode->setCursor( Cursor::Arrow );
		}
	} else if ( Control == mBorderBottom ) {
		if ( Pos.x < mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( Cursor::SizeNESW ); // RESIZE_LEFTBOTTOM
		} else if ( Pos.x > getSize().getWidth() - mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( Cursor::SizeNWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			sceneNode->setCursor( Cursor::SizeNS ); // RESIZE_BOTTOM
		}
	} else if ( Control == mBorderLeft )  {
		if ( Pos.y >= getSize().getHeight() - mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( Cursor::SizeNESW ); // RESIZE_LEFTBOTTOM
		} else {
			sceneNode->setCursor( Cursor::SizeWE ); // RESIZE_LEFT
		}
	} else if ( Control == mBorderRight ) {
		if ( Pos.y >= getSize().getHeight() - mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( Cursor::SizeNWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			sceneNode->setCursor( Cursor::SizeWE ); // RESIZE_RIGHT
		}
	}
}

bool UIWindow::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	if ( "width" == name ) {
		setSize( attribute.asDpDimension(), getSize().getHeight() );
	} else if ( "height" == name ) {
		setSize( getSize().getWidth(), attribute.asDpDimension() );
	} else if ( "title" == name ) {
		setTitle( attribute.asString() );
	} else if ( "base-alpha" == name || "basealpha" == name ) {
		unsigned int val = attribute.asUint();
		if ( val <= 255 )
			setBaseAlpha( (Uint8)val );
	} else if ( "buttons-position-offset" == name || "buttonspositionoffset" == name ) {
		mStyleConfig.ButtonsPositionFixer = attribute.asVector2i();
		fixChildsSize();
	} else if ( "window-flags" == name || "winflags" == name ) {
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
				else if ( "noborder" == cur || "borderless" == cur ) winflags |= UI_WIN_NO_DECORATION;
				else if ( "resizeable" == cur ) winflags |= UI_WIN_RESIZEABLE;
				else if ( "sharealpha" == cur ) winflags |= UI_WIN_SHARE_ALPHA_WITH_CHILDS;
				else if ( "buttonactions" == cur ) winflags |= UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS;
				else if ( "framebuffer"== cur ) winflags |= UI_WIN_FRAME_BUFFER;
				else if ( "colorbuffer"== cur ) winflags |= UI_WIN_COLOR_BUFFER;
			}

			/// TODO: WinFlags should replace old winFlags
			mStyleConfig.WinFlags |= winflags;
			updateWinFlags();
		}
	} else if ( "decoration-size" == name || "decorationsize" == name ) {
		mStyleConfig.DecorationSize = attribute.asSizei();
		fixChildsSize();
	} else if ( "border-size" == name || "bordersize" == name ) {
		mStyleConfig.BorderSize = attribute.asSizei();
		fixChildsSize();
	} else if ( "min-window-size" == name || "minwindowsize" == name ) {
		mStyleConfig.MinWindowSize = attribute.asSizef();
		fixChildsSize();
	} else if ( "buttons-separation" == name || "buttonsseparation" == name ) {
		mStyleConfig.ButtonsSeparation = attribute.asDpDimensionUint();
		fixChildsSize();
	} else if ( "min-corner-distance" == name || "mincornerdistance" == name ) {
		mStyleConfig.MinCornerDistance = attribute.asDpDimensionI();
	} else if ( "decoration-auto-size" == name || "decorationautosize" == name ) {
		mStyleConfig.DecorationAutoSize = attribute.asBool();
		fixChildsSize();
	} else if ( "border-auto-size" == name || "borderautosize" == name ) {
		mStyleConfig.BorderAutoSize = attribute.asBool();
		fixChildsSize();
	} else {
		return UIWidget::setAttribute( attribute, state );
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
