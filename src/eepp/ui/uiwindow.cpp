#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwindow.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIWindow* UIWindow::NewOpt( UIWindow::WindowBaseContainerType type,
							const StyleConfig& windowStyleConfig ) {
	return eeNew( UIWindow, ( type, windowStyleConfig ) );
}

UIWindow* UIWindow::New() {
	return eeNew( UIWindow, ( SIMPLE_LAYOUT ) );
}

UIWindow::UIWindow( UIWindow::WindowBaseContainerType type ) : UIWindow( type, StyleConfig() ) {}

UIWindow::UIWindow( UIWindow::WindowBaseContainerType type, const StyleConfig& windowStyleConfig ) :
	UIWidget( "window" ),
	mFrameBuffer( NULL ),
	mStyleConfig( windowStyleConfig ),
	mWindowDecoration( NULL ),
	mBorderLeft( NULL ),
	mBorderRight( NULL ),
	mBorderBottom( NULL ),
	mContainer( NULL ),
	mButtonClose( NULL ),
	mButtonMinimize( NULL ),
	mButtonMaximize( NULL ),
	mTitle( NULL ),
	mModalNode( NULL ),
	mResizeType( RESIZE_NONE ),
	mFrameBufferBound( false ),
	mKeyBindings( getEventDispatcher()->getInput() ) {
	subscribeScheduledUpdate();

	bool loading = isSceneNodeLoading();
	mUISceneNode->setIsLoading( true );

	mNodeFlags |= NODE_FLAG_WINDOW | NODE_FLAG_VIEW_DIRTY;
	mFlags |= UI_OWNS_CHILDS_POSITION;

	setHorizontalAlign( UI_HALIGN_CENTER );

	if ( NULL != getUISceneNode() )
		getUISceneNode()->windowAdd( this );

	switch ( type ) {
		case LINEAR_LAYOUT:
			mContainer = UILinearLayout::NewWithTag( "window::container", UIOrientation::Vertical );
			break;
		case RELATIVE_LAYOUT:
			mContainer = UIRelativeLayout::NewWithTag( "window::container" );
			break;
		case SIMPLE_LAYOUT:
		default:
			mContainer = UIWidget::NewWithTag( "window::container" );
			break;
	}

	setId( "window" );

	mContainer->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mContainer->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
	mContainer->setParent( this );
	mContainer->setClipType( ClipType::ContentBox );
	mContainer->enableReportSizeChangeToChilds();
	mContainer->addEventListener( Event::OnPositionChange,
								  [this]( auto event ) { onContainerPositionChange( event ); } );

	updateWinFlags();

	setAlpha( mStyleConfig.BaseAlpha );

	applyDefaultTheme();

	runOnMainThread( [this]() { onWindowReady(); } );

	mUISceneNode->setIsLoading( loading );
}

UIWindow::~UIWindow() {
	if ( NULL != getUISceneNode() && !SceneManager::instance()->isShuttingDown() ) {
		if ( NULL != mModalNode ) {
			mModalNode->setEnabled( false );
			mModalNode->setVisible( false );
			mModalNode->close();
		}

		getUISceneNode()->windowRemove( this );

		if ( isParentOf( getEventDispatcher()->getFocusNode() ) )
			getUISceneNode()->setFocusLastWindow( this );
	}

	sendCommonEvent( Event::OnWindowClose );

	eeSAFE_DELETE( mFrameBuffer );
}

void UIWindow::onContainerPositionChange( const Event* ) {
	if ( NULL == mContainer )
		return;

	Vector2f PosDiff =
		mContainer->getPosition() -
		Vector2f(
			NULL != mBorderLeft ? mBorderLeft->getSize().getWidth() + mPadding.Left : mPadding.Left,
			NULL != mWindowDecoration ? mWindowDecoration->getSize().getHeight() + mPadding.Top
									  : mPadding.Top );

	if ( PosDiff.x != 0 || PosDiff.y != 0 ) {
		mContainer->setPosition(
			NULL != mBorderLeft ? mBorderLeft->getSize().getWidth() : 0,
			NULL != mWindowDecoration ? mWindowDecoration->getSize().getHeight() : 0 );

		setPosition( mDpPos + PosDiff );
	}
}

void UIWindow::updateWinFlags() {
	bool needsUpdate = false;

	writeNodeFlag( NODE_FLAG_FRAME_BUFFER,
				   ( mStyleConfig.WinFlags & UI_WIN_FRAME_BUFFER ) ? 1 : 0 );

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
			mWindowDecoration = UIWidget::NewWithTag( "window::decoration" );
			mWindowDecoration->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		auto cb = [this]( const Event* ) { onSizeChange(); };

		mWindowDecoration->setParent( this );
		mWindowDecoration->setVisible( true );
		mWindowDecoration->setEnabled( false );

		if ( NULL == mBorderLeft ) {
			mBorderLeft = UIWidget::NewWithTag( "window::border::left" );
			mBorderLeft->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mBorderLeft->setParent( this );
		mBorderLeft->setEnabled( true );
		mBorderLeft->setVisible( true );

		if ( NULL == mBorderRight ) {
			mBorderRight = UIWidget::NewWithTag( "window::border::right" );
			mBorderRight->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mBorderRight->setParent( this );
		mBorderRight->setEnabled( true );
		mBorderRight->setVisible( true );

		if ( NULL == mBorderBottom ) {
			mBorderBottom = UIWidget::NewWithTag( "window::border::bottom" );
			mBorderBottom->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		}

		mBorderBottom->setParent( this );
		mBorderBottom->setEnabled( true );
		mBorderBottom->setVisible( true );

		if ( mStyleConfig.WinFlags & UI_WIN_CLOSE_BUTTON ) {
			if ( NULL == mButtonClose ) {
				mButtonClose = UIWidget::NewWithTag( "window::close" );
				mButtonClose->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
				mButtonClose->addEventListener( Event::OnSizeChange, cb );
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
				mButtonMaximize = UIWidget::NewWithTag( "window::maximize" );
				mButtonMaximize->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
				mButtonMaximize->addEventListener( Event::OnSizeChange, cb );
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
				mButtonMinimize = UIWidget::NewWithTag( "window::minimize" );
				mButtonMinimize->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
				mButtonMinimize->addEventListener( Event::OnSizeChange, cb );
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

	if ( isModal() && NULL == mModalNode ) {
		createModalNode();
	}

	if ( needsUpdate ) {
		applyDefaultTheme();
	}
}

void UIWindow::createFrameBuffer() {
	eeSAFE_DELETE( mFrameBuffer );
	Sizei fboSize( getFrameBufferSize() );
	if ( fboSize.getWidth() < 1 )
		fboSize.setWidth( 1 );
	if ( fboSize.getHeight() < 1 )
		fboSize.setHeight( 1 );
	mFrameBuffer =
		FrameBuffer::New( fboSize.getWidth(), fboSize.getHeight(), true, false,
						  ( mStyleConfig.WinFlags & UI_WIN_COLOR_BUFFER ) ? true : false );

	// Frame buffer failed to create?
	if ( !mFrameBuffer->created() ) {
		eeSAFE_DELETE( mFrameBuffer );
	}
}

void UIWindow::drawFrameBuffer() {
	if ( NULL != mFrameBuffer ) {
		if ( mFrameBuffer->hasColorBuffer() ) {
			mFrameBuffer->draw( Rect( 0, 0, mSize.getWidth(), mSize.getHeight() ),
								Rect( mScreenPos.x, mScreenPos.y, mScreenPos.x + mSize.getWidth(),
									  mScreenPos.y + mSize.getHeight() ) );
		} else {
			Rect r( 0, 0, mSize.getWidth(), mSize.getHeight() );
			TextureRegion textureRegion( mFrameBuffer->getTexture(), r, r.getSize().asFloat() );
			textureRegion.draw( mScreenPosi.x, mScreenPosi.y, Color::White, getRotation(),
								getScale() );
		}
	}
}

void UIWindow::drawHighlightInvalidation() {
	if ( ( mNodeFlags & NODE_FLAG_VIEW_DIRTY ) && NULL != mSceneNode &&
		 mSceneNode->getHighlightInvalidation() ) {
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

		P.drawRectangle( Rectf( Vector2f( ShadowPos.x, ShadowPos.y ),
								Sizef( mSize.getWidth(), mSize.getHeight() ) ),
						 BeginC, BeginC, BeginC, BeginC );

		P.drawRectangle(
			Rectf( Vector2f( ShadowPos.x, ShadowPos.y - SSize ), Sizef( mSize.getWidth(), SSize ) ),
			EndC, BeginC, BeginC, EndC );

		P.drawRectangle( Rectf( Vector2f( ShadowPos.x - SSize, ShadowPos.y ),
								Sizef( SSize, mSize.getHeight() ) ),
						 EndC, EndC, BeginC, BeginC );

		P.drawRectangle( Rectf( Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y ),
								Sizef( SSize, mSize.getHeight() ) ),
						 BeginC, BeginC, EndC, EndC );

		P.drawRectangle( Rectf( Vector2f( ShadowPos.x, ShadowPos.y + mSize.getHeight() ),
								Sizef( mSize.getWidth(), SSize ) ),
						 BeginC, EndC, EndC, BeginC );

		P.drawTriangle(
			Triangle2f( Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y ),
						Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y - SSize ),
						Vector2f( ShadowPos.x + mSize.getWidth() + SSize, ShadowPos.y ) ),
			BeginC, EndC, EndC );

		P.drawTriangle( Triangle2f( Vector2f( ShadowPos.x, ShadowPos.y ),
									Vector2f( ShadowPos.x, ShadowPos.y - SSize ),
									Vector2f( ShadowPos.x - SSize, ShadowPos.y ) ),
						BeginC, EndC, EndC );

		P.drawTriangle(
			Triangle2f(
				Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y + mSize.getHeight() ),
				Vector2f( ShadowPos.x + mSize.getWidth(), ShadowPos.y + mSize.getHeight() + SSize ),
				Vector2f( ShadowPos.x + mSize.getWidth() + SSize,
						  ShadowPos.y + mSize.getHeight() ) ),
			BeginC, EndC, EndC );

		P.drawTriangle(
			Triangle2f( Vector2f( ShadowPos.x, ShadowPos.y + mSize.getHeight() ),
						Vector2f( ShadowPos.x - SSize, ShadowPos.y + mSize.getHeight() ),
						Vector2f( ShadowPos.x, ShadowPos.y + mSize.getHeight() + SSize ) ),
			BeginC, EndC, EndC );

		P.setForceDraw( true );

		UIWidget::matrixUnset();
	}
}

void UIWindow::onPaddingChange() {
	fixChildsSize();

	UIWidget::onPaddingChange();
}

Sizei UIWindow::getFrameBufferSize() {
	return isResizeable() && (Node*)this != mSceneNode
			   ? Sizei( Math::nextPowOfTwo( (int)mSize.getWidth() ),
						Math::nextPowOfTwo( (int)mSize.getHeight() ) )
			   : mSize.ceil().asInt();
}

void UIWindow::forcedApplyStyle() {
	if ( NULL != mStyle ) {
		mStyle->setForceReapplyProperties( true );
		mStyle->setDisableAnimations( true );
		reportStyleStateChange();
		mStyle->setDisableAnimations( false );
	}
}

void UIWindow::onWindowReady() {
	mWindowReady = true;

	forcedApplyStyle();

	if ( mShowWhenReady ) {
		mShowWhenReady = false;
		show();
	}

	sendCommonEvent( Event::OnWindowReady );
}

void UIWindow::createModalNode() {
	Node* node = mSceneNode;

	if ( NULL == node )
		return;

	if ( NULL == mModalNode ) {
		mModalNode = UIWidget::NewWithTag( "window::modaldialog" );
		mModalNode->setParent( node )->setPosition( 0, 0 )->setSize( node->getSize() );
		mModalNode->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT |
								UI_ANCHOR_BOTTOM );
	} else {
		mModalNode->setPosition( 0, 0 );
		mModalNode->setSize( node->getSize() );
		mModalNode->updateAnchorsDistances();
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

	UIThemeManager* themeManager = getUISceneNode()->getUIThemeManager();
	if ( themeManager->getDefaultEffectsEnabled() ) {
		runAction(
			Actions::Sequence::New( Actions::FadeOut::New( themeManager->getWidgetsFadeOutTime() ),
									Actions::Close::New() ) );
	} else {
		close();
	}
}

void UIWindow::close() {
	UIWidget::close();

	if ( NULL != mModalNode ) {
		mModalNode->setEnabled( false );
		mModalNode->setVisible( false );
		mModalNode->close();
		mModalNode = NULL;
	}
}

void UIWindow::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	if ( NULL != mContainer )
		mContainer->setThemeSkin( Theme, "winback" );

	if ( !( mStyleConfig.WinFlags & UI_WIN_NO_DECORATION ) ) {
		mWindowDecoration->setThemeSkin( Theme, "windeco" );
		mBorderLeft->setThemeSkin( Theme, "winborderleft" );
		mBorderRight->setThemeSkin( Theme, "winborderright" );
		mBorderBottom->setThemeSkin( Theme, "winborderbottom" );

		if ( NULL != mButtonClose ) {
			mButtonClose->setThemeSkin( Theme, "winclose" );
			if ( mButtonClose->getSkinSize() != Sizef::Zero )
				mButtonClose->setSize( mButtonClose->getSkinSize() );
		}

		if ( NULL != mButtonMaximize ) {
			mButtonMaximize->setThemeSkin( Theme, "winmax" );
			if ( mButtonMaximize->getSkinSize() != Sizef::Zero )
				mButtonMaximize->setSize( mButtonMaximize->getSkinSize() );
		}

		if ( NULL != mButtonMinimize ) {
			mButtonMinimize->setThemeSkin( Theme, "winmin" );
			if ( mButtonMinimize->getSkinSize() != Sizef::Zero )
				mButtonMinimize->setSize( mButtonMinimize->getSkinSize() );
		}

		calcMinWinSize();
	}

	fixChildsSize();
	onThemeLoaded();
}

void UIWindow::calcMinWinSize() {
	if ( NULL == mWindowDecoration ||
		 ( mStyleConfig.MinWindowSize.x != 0 && mStyleConfig.MinWindowSize.y != 0 ) )
		return;

	Sizei tSize;

	tSize.x = mBorderLeft->getSize().getWidth() + mBorderRight->getSize().getWidth() -
			  mStyleConfig.ButtonsOffset.x;
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
	Sizef size( getMinWindowSizeWithDecoration() );

	if ( getSize().getWidth() < size.getWidth() && getSize().getHeight() < size.getHeight() ) {
		setSize( mStyleConfig.MinWindowSize );
	} else if ( getSize().getWidth() < size.getWidth() ) {
		setSize( Sizef( mStyleConfig.MinWindowSize.getWidth(), getSize().getHeight() ) );
	} else if ( getSize().getHeight() < size.getHeight() ) {
		setSize( Sizef( getSize().getWidth(), mStyleConfig.MinWindowSize.getHeight() ) );
	}
}

void UIWindow::onSizeChange() {
	Sizef size( getMinWindowSizeWithDecoration() );

	if ( getSize().getWidth() < size.getWidth() || getSize().getHeight() < size.getHeight() ) {
		if ( getSize().getWidth() < size.getWidth() &&
			 getSize().getHeight() < mStyleConfig.MinWindowSize.getHeight() ) {
			setSize( mStyleConfig.MinWindowSize );
		} else if ( getSize().getWidth() < size.getWidth() ) {
			setSize( Sizef( mStyleConfig.MinWindowSize.getWidth(), getSize().getHeight() ) );
		} else if ( getSize().getHeight() < size.getHeight() ) {
			setSize( Sizef( getSize().getWidth(), mStyleConfig.MinWindowSize.getHeight() ) );
		}
	} else {
		fixChildsSize();

		if ( ownsFrameBuffer() && NULL != mFrameBuffer &&
			 ( mFrameBuffer->getWidth() < mSize.getWidth() ||
			   mFrameBuffer->getHeight() < mSize.getHeight() ) ) {
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

UINode* UIWindow::setSize( const Sizef& Size ) {
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

UINode* UIWindow::setSize( const Float& Width, const Float& Height ) {
	setSize( Sizef( Width, Height ) );
	return this;
}

UIWindow* UIWindow::setSizeWithDecoration( const Float& Width, const Float& Height ) {
	setSizeWithDecoration( Sizef( Width, Height ) );
	return this;
}

UIWindow* UIWindow::setSizeWithDecoration( const Sizef& size ) {
	UIWidget::setSize( size );
	return this;
}

const Sizef& UIWindow::getSize() const {
	return UIWidget::getSize();
}

void UIWindow::fixChildsSize() {
	Sizef size( PixelDensity::dpToPx( getMinWindowSizeWithDecoration() ) );

	if ( mSize.getWidth() < size.getWidth() || mSize.getHeight() < size.getHeight() ) {
		internalSize( eemin( mSize.getWidth(), size.getWidth() ),
					  eemin( mSize.getHeight(), size.getHeight() ) );
	}

	if ( NULL == mWindowDecoration && NULL != mContainer ) {
		mContainer->setPixelsSize( mSize - mPaddingPx );
		mContainer->setPixelsPosition( mPaddingPx.Left, mPaddingPx.Top );
		return;
	}

	Sizei decoSize = mStyleConfig.TitlebarSize;

	if ( mStyleConfig.TitlebarAutoSize ) {
		decoSize = mStyleConfig.TitlebarSize =
			Sizei( getSize().getWidth(), mWindowDecoration->getSkinSize().getHeight() );
	}

	if ( mWindowDecoration )
		mWindowDecoration->setPixelsSize(
			mSize.getWidth(), PixelDensity::dpToPx( mStyleConfig.TitlebarSize.getHeight() ) );

	if ( mBorderBottom ) {
		if ( mStyleConfig.BorderAutoSize ) {
			mBorderBottom->setPixelsSize(
				mSize.getWidth(),
				PixelDensity::dpToPx( mBorderBottom->getSkinSize().getHeight() ) );
		} else {
			mBorderBottom->setPixelsSize(
				mSize.getWidth(), PixelDensity::dpToPx( mStyleConfig.BorderSize.getHeight() ) );
		}
	}

	Uint32 BorderHeight = mSize.getHeight() - PixelDensity::dpToPx( decoSize.getHeight() ) -
						  mBorderBottom->getPixelsSize().getHeight();

	if ( mStyleConfig.BorderAutoSize ) {
		mBorderLeft->setPixelsSize( PixelDensity::dpToPx( mBorderLeft->getSkinSize().getWidth() ),
									BorderHeight );
		mBorderRight->setPixelsSize( PixelDensity::dpToPx( mBorderRight->getSkinSize().getWidth() ),
									 BorderHeight );
	} else {
		mBorderLeft->setPixelsSize( PixelDensity::dpToPx( mStyleConfig.BorderSize.getWidth() ),
									BorderHeight );
		mBorderRight->setPixelsSize( PixelDensity::dpToPx( mStyleConfig.BorderSize.getWidth() ),
									 BorderHeight );
	}

	mBorderLeft->setPixelsPosition( 0, mWindowDecoration->getPixelsSize().getHeight() );
	mBorderRight->setPixelsPosition( mSize.getWidth() - mBorderRight->getPixelsSize().getWidth(),
									 mWindowDecoration->getPixelsSize().getHeight() );
	mBorderBottom->setPixelsPosition( 0, mWindowDecoration->getPixelsSize().getHeight() +
											 mBorderLeft->getPixelsSize().getHeight() );

	mContainer->setPixelsPosition( mBorderLeft->getPixelsSize().getWidth() + mPaddingPx.Left,
								   mWindowDecoration->getPixelsSize().getHeight() +
									   mPaddingPx.Top );
	mContainer->setPixelsSize(
		mSize.getWidth() - mBorderLeft->getPixelsSize().getWidth() -
			mBorderRight->getPixelsSize().getWidth() - mPaddingPx.Left - mPaddingPx.Right,
		mSize.getHeight() - mWindowDecoration->getPixelsSize().getHeight() -
			mBorderBottom->getPixelsSize().getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );

	Uint32 yPos;
	Vector2f posFix( PixelDensity::dpToPx(
		Vector2f( mStyleConfig.ButtonsOffset.x, mStyleConfig.ButtonsOffset.y ) ) );

	if ( NULL != mButtonClose ) {
		yPos = mWindowDecoration->getPixelsSize().getHeight() / 2 -
			   mButtonClose->getPixelsSize().getHeight() / 2 + posFix.y;

		mButtonClose->setPixelsPosition( mWindowDecoration->getPixelsSize().getWidth() -
											 mBorderRight->getPixelsSize().getWidth() -
											 mButtonClose->getPixelsSize().getWidth() + posFix.x,
										 yPos );
	}

	if ( NULL != mButtonMaximize ) {
		yPos = mWindowDecoration->getPixelsSize().getHeight() / 2 -
			   mButtonMaximize->getPixelsSize().getHeight() / 2 + posFix.y;

		if ( NULL != mButtonClose ) {
			mButtonMaximize->setPixelsPosition(
				mButtonClose->getPixelsPosition().x -
					PixelDensity::dpToPx( mStyleConfig.ButtonsSeparation ) -
					mButtonMaximize->getPixelsSize().getWidth(),
				yPos );
		} else {
			mButtonMaximize->setPixelsPosition( mWindowDecoration->getPixelsSize().getWidth() -
													mBorderRight->getPixelsSize().getWidth() -
													mButtonMaximize->getPixelsSize().getWidth() +
													posFix.x,
												yPos );
		}
	}

	if ( NULL != mButtonMinimize ) {
		yPos = mWindowDecoration->getPixelsSize().getHeight() / 2 -
			   mButtonMinimize->getPixelsSize().getHeight() / 2 + posFix.y;

		if ( NULL != mButtonMaximize ) {
			mButtonMinimize->setPixelsPosition(
				mButtonMaximize->getPixelsPosition().x -
					PixelDensity::dpToPx( mStyleConfig.ButtonsSeparation ) -
					mButtonMinimize->getPixelsSize().getWidth(),
				yPos );
		} else {
			if ( NULL != mButtonClose ) {
				mButtonMinimize->setPixelsPosition(
					mButtonClose->getPixelsPosition().x -
						PixelDensity::dpToPx( mStyleConfig.ButtonsSeparation ) -
						mButtonMinimize->getPixelsSize().getWidth(),
					yPos );
			} else {
				mButtonMinimize->setPixelsPosition(
					mWindowDecoration->getPixelsSize().getWidth() -
						mBorderRight->getPixelsSize().getWidth() -
						mButtonMinimize->getPixelsSize().getWidth() + posFix.x,
					yPos );
			}
		}
	}

	fixTitleSize();
}

Uint32 UIWindow::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::Focus: {
			sendWindowToFront();
			break;
		}
		case NodeMessage::FocusLoss: {
			checkEphemeralClose();
			break;
		}
		case NodeMessage::MouseDown: {
			doResize( Msg );
			break;
		}
		case NodeMessage::WindowResize: {
			if ( isModal() && NULL != mModalNode && NULL != mSceneNode ) {
				mModalNode->setSize( mSceneNode->getSize() );
			}

			break;
		}
		case NodeMessage::MouseLeave: {
			if ( getUISceneNode() != NULL )
				getUISceneNode()->setCursor( Cursor::Arrow );
			break;
		}
		case NodeMessage::DragStart: {
			if ( getUISceneNode() != NULL )
				getUISceneNode()->setCursor( Cursor::Hand );

			sendWindowToFront();

			break;
		}
		case NodeMessage::DragStop: {
			if ( getUISceneNode() != NULL )
				getUISceneNode()->setCursor( Cursor::Arrow );
			break;
		}
		case NodeMessage::MouseClick: {
			if ( ( mStyleConfig.WinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) &&
				 ( Msg->getFlags() & EE_BUTTON_LMASK ) ) {
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

void UIWindow::doResize( const NodeMessage* Msg ) {
	if ( NULL == mWindowDecoration || NULL == getEventDispatcher() )
		return;

	if ( !isResizeable() || !( Msg->getFlags() & EE_BUTTON_LMASK ) || RESIZE_NONE != mResizeType ||
		 ( getEventDispatcher()->getLastPressTrigger() & EE_BUTTON_LMASK ) )
		return;

	decideResizeType( Msg->getSender() );
}

void UIWindow::decideResizeType( Node* node ) {
	if ( NULL == getEventDispatcher() )
		return;

	Vector2i Pos = getEventDispatcher()->getMousePos();

	worldToNode( Pos );

	if ( node == this ) {
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
	} else if ( node == mBorderBottom ) {
		if ( Pos.x < mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_LEFTBOTTOM );
		} else if ( Pos.x > getSize().getWidth() - mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_RIGHTBOTTOM );
		} else {
			tryResize( RESIZE_BOTTOM );
		}
	} else if ( node == mBorderLeft ) {
		if ( Pos.y >= getSize().getHeight() - mStyleConfig.MinCornerDistance ) {
			tryResize( RESIZE_LEFTBOTTOM );
		} else {
			tryResize( RESIZE_LEFT );
		}
	} else if ( node == mBorderRight ) {
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

	switch ( mResizeType ) {
		case RESIZE_RIGHT: {
			mResizePos.x = mSize.getWidth() - Pos.x;
			break;
		}
		case RESIZE_LEFT: {
			mResizePos.x = Pos.x;
			break;
		}
		case RESIZE_TOP: {
			mResizePos.y = Pos.y;
			break;
		}
		case RESIZE_BOTTOM: {
			mResizePos.y = mSize.getHeight() - Pos.y;
			break;
		}
		case RESIZE_RIGHTBOTTOM: {
			mResizePos.x = mSize.getWidth() - Pos.x;
			mResizePos.y = mSize.getHeight() - Pos.y;
			break;
		}
		case RESIZE_LEFTBOTTOM: {
			mResizePos.x = Pos.x;
			mResizePos.y = mSize.getHeight() - Pos.y;
			break;
		}
		case RESIZE_TOPLEFT: {
			mResizePos.x = Pos.x;
			mResizePos.y = Pos.y;
			break;
		}
		case RESIZE_TOPRIGHT: {
			mResizePos.y = Pos.y;
			mResizePos.x = mSize.getWidth() - Pos.x;
			break;
		}
		case RESIZE_NONE: {
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
		case RESIZE_RIGHT: {
			internalSize( Pos.x + mResizePos.x, mSize.getHeight() );
			break;
		}
		case RESIZE_BOTTOM: {
			internalSize( mSize.getWidth(), Pos.y + mResizePos.y );
			break;
		}
		case RESIZE_LEFT: {
			Pos.x -= mResizePos.x;
			UINode::setPixelsPosition( mPosition.x + Pos.x, mPosition.y );
			internalSize( mSize.getWidth() - Pos.x, mSize.getHeight() );
			break;
		}
		case RESIZE_TOP: {
			Pos.y -= mResizePos.y;
			UINode::setPixelsPosition( mPosition.x, mPosition.y + Pos.y );
			internalSize( mSize.getWidth(), mSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_RIGHTBOTTOM: {
			Pos += mResizePos;
			internalSize( Pos.x, Pos.y );
			break;
		}
		case RESIZE_TOPLEFT: {
			Pos -= mResizePos;
			UINode::setPixelsPosition( mPosition.x + Pos.x, mPosition.y + Pos.y );
			internalSize( mSize.getWidth() - Pos.x, mSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_TOPRIGHT: {
			Pos.y -= mResizePos.y;
			Pos.x += mResizePos.x;
			UINode::setPixelsPosition( mPosition.x, mPosition.y + Pos.y );
			internalSize( Pos.x, mSize.getHeight() - Pos.y );
			break;
		}
		case RESIZE_LEFTBOTTOM: {
			Pos.x -= mResizePos.x;
			Pos.y += mResizePos.y;
			UINode::setPixelsPosition( mPosition.x + Pos.x, mPosition.y );
			internalSize( mSize.getWidth() - Pos.x, Pos.y );
			break;
		}
		case RESIZE_NONE: {
		}
	}
}

void UIWindow::internalSize( const Float& w, const Float& h ) {
	internalSize( Sizef( w, h ) );
}

void UIWindow::internalSize( Sizef Size ) {
	Sizef realMin = PixelDensity::dpToPx( getMinWindowSizeWithDecoration() );

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

UIWidget* UIWindow::getContainer() const {
	return mContainer;
}

UINode* UIWindow::getButtonClose() const {
	return mButtonClose;
}

UINode* UIWindow::getButtonMaximize() const {
	return mButtonMaximize;
}

UINode* UIWindow::getButtonMinimize() const {
	return mButtonMinimize;
}

void UIWindow::setupModal() {
	if ( isModal() ) {
		createModalNode();

		mModalNode->setEnabled( true );
		mModalNode->setVisible( true );
		mModalNode->toFront();

		sendWindowToFront();
	}
}

bool UIWindow::show() {
	if ( !isVisible() ) {
		setEnabled( true );
		setVisible( true );

		setFocus();

		UIThemeManager* themeManager = getUISceneNode()->getUIThemeManager();
		if ( themeManager->getDefaultEffectsEnabled() ) {
			runAction( Actions::Fade::New( mStyleConfig.BaseAlpha == getAlpha() ? 0.f : mAlpha,
										   mStyleConfig.BaseAlpha,
										   themeManager->getWidgetsFadeOutTime() ) );
		}
		setupModal();

		return true;
	}

	setupModal();

	return false;
}

bool UIWindow::hide() {
	if ( isVisible() ) {
		UIThemeManager* themeManager = getUISceneNode()->getUIThemeManager();
		if ( themeManager->getDefaultEffectsEnabled() ) {
			runAction( Actions::Sequence::New(
				Actions::FadeOut::New( themeManager->getWidgetsFadeOutTime() ),
				Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ) ) ) );
		} else {
			setEnabled( false );
			setVisible( false );
		}

		if ( NULL != mSceneNode )
			mSceneNode->setFocus();

		if ( NULL != mModalNode ) {
			mModalNode->setEnabled( false );
			mModalNode->setVisible( false );
		}

		return true;
	}

	return false;
}

UIWindow* UIWindow::showWhenReady() {
	if ( mWindowReady ) {
		show();
	} else {
		mShowWhenReady = true;
		hide();
	}
	return this;
}

void UIWindow::onAlphaChange() {
	if ( mStyleConfig.WinFlags & UI_WIN_SHARE_ALPHA_WITH_CHILDS ) {
		Node* CurChild = mChild;

		while ( NULL != CurChild ) {
			CurChild->setAlpha( mAlpha );
			CurChild = CurChild->getNextNode();
		}
	}

	UIWidget::onAlphaChange();
}

void UIWindow::onChildCountChange( Node* child, const bool& removed ) {
	if ( NULL == mContainer )
		return;

	if ( !removed && !( child->getNodeFlags() & NODE_FLAG_OWNED_BY_NODE ) ) {
		child->setParent( mContainer );
	}
}

void UIWindow::onPositionChange() {
	// Invalidate the buffer since a position change can get childs into a drawable position
	// (on screen), when the drawable could have been outside the viewport and not drawn in the
	// previous position.
	invalidate( this );

	UIWidget::onPositionChange();
}

void UIWindow::setWindowOpacity( const Uint8& Alpha ) {
	if ( mAlpha == mStyleConfig.BaseAlpha ) {
		UINode::setAlpha( Alpha );
	}

	mStyleConfig.BaseAlpha = Alpha;
}

const Uint8& UIWindow::getBaseAlpha() const {
	return mStyleConfig.BaseAlpha;
}

UIWindow* UIWindow::setTitle( const String& text ) {
	if ( mTitle && text == mTitle->getText() )
		return this;

	if ( NULL == mTitle ) {
		mTitle = UITextView::NewWithTag( "window::title" );
		mTitle->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		mTitle->writeNodeFlag( NODE_FLAG_OWNED_BY_NODE, 1 );
		mTitle->setParent( this );
		mTitle->setHorizontalAlign( getHorizontalAlign() );
		mTitle->setVerticalAlign( getVerticalAlign() );
		mTitle->setEnabled( false );
		mTitle->setClipType( ClipType::ContentBox );
		mTitle->setVisible( !( mStyleConfig.WinFlags & UI_WIN_NO_DECORATION ) );
	}

	fixTitleSize();

	mTitle->setText( text );
	sendTextEvent( Event::OnTitleChange, text.toUtf8() );
	return this;
}

void UIWindow::fixTitleSize() {
	if ( NULL != mWindowDecoration && NULL != mTitle ) {
		mTitle->setSize( mWindowDecoration->getSize().getWidth() -
							 mBorderLeft->getSize().getWidth() - mBorderRight->getSize().getWidth(),
						 mWindowDecoration->getSize().getHeight() );
		mTitle->setPosition( mBorderLeft->getSize().getWidth(), 0 );
	}
}

String UIWindow::getTitle() const {
	if ( NULL != mTitle )
		return mTitle->getText();

	return String();
}

UITextView* UIWindow::getTitleTextBox() const {
	return mTitle;
}

void UIWindow::maximize() {
	Node* node = mSceneNode;

	if ( NULL == node )
		return;

	if ( node->getSize() == getSize() ) {
		setPixelsPosition( mNonMaxPos );
		internalSize( mNonMaxSize );
	} else {
		mNonMaxPos = mPosition;
		mNonMaxSize = mSize;

		setPosition( 0, 0 );
		setSizeWithDecoration( node->getSize() );
	}
}

Uint32 UIWindow::onMouseDoubleClick( const Vector2i&, const Uint32& Flags ) {
	if ( isResizeable() && ( NULL != mButtonMaximize ) && ( Flags & EE_BUTTON_LMASK ) ) {
		maximize();

		sendCommonEvent( Event::OnWindowMaximizeClick );
	}

	return 1;
}

void UIWindow::nodeDraw() {
	if ( mVisible && 0 != mAlpha ) {
		if ( mNodeFlags & NODE_FLAG_POSITION_DIRTY )
			updateScreenPos();

		if ( mNodeFlags & NODE_FLAG_POLYGON_DIRTY )
			updateWorldPolygon();

		preDraw();

		drawShadow();

		ClippingMask* clippingMask = GLi->getClippingMask();

		const std::vector<Rectf>& clips = clippingMask->getPlanesClipped();

		if ( !clips.empty() )
			clippingMask->clipPlaneDisable();

		matrixSet();

		smartClipStart( ClipType::BorderBox );

		if ( !ownsFrameBuffer() || ( NULL != mSceneNode && !mSceneNode->usesInvalidation() ) ||
			 invalidated() ) {
			smartClipStart( ClipType::ContentBox );

			if ( 0.f != mAlpha ) {
				drawBackground();

				drawSkin();
			}

			smartClipStart( ClipType::PaddingBox );

			draw();

			drawChilds();

			smartClipEnd( ClipType::PaddingBox );

			if ( 0.f != mAlpha )
				drawForeground();

			smartClipEnd( ClipType::ContentBox );
		}

		drawBorder();

		if ( mNodeFlags & NODE_FLAG_DROPPABLE_HOVERING )
			drawDroppableHovering();

		drawHighlightFocus();

		drawOverNode();

		updateDebugData();

		drawBox();

		smartClipEnd( ClipType::BorderBox );

		matrixUnset();

		if ( !clips.empty() )
			clippingMask->setPlanesClipped( clips );

		postDraw();

		drawHighlightInvalidation();

		writeNodeFlag( NODE_FLAG_VIEW_DIRTY, 0 );
	}
}

void UIWindow::invalidate( Node* invalidator ) {
	if ( mVisible && mAlpha != 0.f ) {
		writeNodeFlag( NODE_FLAG_VIEW_DIRTY, 1 );

		if ( NULL != mSceneNode )
			mSceneNode->invalidate( invalidator );
	}
}

FrameBuffer* UIWindow::getFrameBuffer() const {
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
				GLi->translatef( -mScreenPosi.x, -mScreenPosi.y, 0.f );
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

Uint32 UIWindow::onFocusLoss() {
	checkEphemeralClose();
	return UIWidget::onFocusLoss();
}

bool UIWindow::ownsFrameBuffer() {
	return 0 != ( mStyleConfig.WinFlags & UI_WIN_FRAME_BUFFER );
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

UIWindow* UIWindow::setWindowFlags( const Uint32& winFlags ) {
	mStyleConfig.WinFlags = winFlags;

	updateWinFlags();

	return this;
}

const UIWindow::StyleConfig& UIWindow::getStyleConfig() const {
	return mStyleConfig;
}

UIWindow* UIWindow::setStyleConfig( const StyleConfig& styleConfig ) {
	mStyleConfig = styleConfig;

	updateWinFlags();

	setAlpha( mStyleConfig.BaseAlpha );

	applyDefaultTheme();

	applyMinWinSize();

	return this;
}

UIWindow* UIWindow::setMinWindowSize( const Float& width, const Float& height ) {
	return setMinWindowSize( Sizef( width, height ) );
}

UIWindow* UIWindow::setMinWindowSize( Sizef size ) {
	if ( mStyleConfig.MinWindowSize != size ) {
		mStyleConfig.MinWindowSize = size;

		applyMinWinSize();
	}
	return this;
}

Sizef UIWindow::getMinWindowSizeWithDecoration() {
	Sizef size( getMinWindowSize() );
	if ( NULL != mWindowDecoration ) {
		size.x += mBorderLeft->getSize().getWidth() + mBorderRight->getSize().getWidth();
		size.y += mWindowDecoration->getSize().getHeight() + mBorderBottom->getSize().getHeight();
	}
	return size;
}

Sizef UIWindow::getSizeWithoutDecoration() {
	Sizef size( getSize() );
	if ( NULL != mWindowDecoration ) {
		size.x -= mBorderLeft->getSize().getWidth() + mBorderRight->getSize().getWidth();
		size.y -= mWindowDecoration->getSize().getHeight() + mBorderBottom->getSize().getHeight();
	}
	return size;
}

Sizef UIWindow::getMinWindowTitleSizeRequired() {
	Sizef size( PixelDensity::pxToDp( mTitle != nullptr
										  ? ( mTitle->getTextWidth() + mTitle->getFontSize() * 4 )
										  : 0 ) +
					mPadding.Left + mPadding.Right,
				0 );

	if ( NULL != mWindowDecoration ) {
		size.x += mBorderLeft->getSize().getWidth() + mBorderRight->getSize().getWidth();
		size.y += mWindowDecoration->getSize().getHeight();
	}

	if ( NULL != mButtonClose )
		size.x += mButtonClose->getSize().getWidth();

	if ( NULL != mButtonMaximize )
		size.x += mButtonMaximize->getSize().getWidth();

	if ( NULL != mButtonMinimize )
		size.x += mButtonMinimize->getSize().getWidth();

	return size;
}

const Sizef& UIWindow::getMinWindowSize() {
	return mStyleConfig.MinWindowSize;
}

bool UIWindow::isModal() {
	return 0 != ( mStyleConfig.WinFlags & UI_WIN_MODAL );
}

UIWidget* UIWindow::getModalWidget() const {
	return mModalNode;
}

void UIWindow::resizeCursor() {
	UISceneNode* sceneNode = getUISceneNode();

	if ( NULL == sceneNode || !isMouseOverMeOrChilds() || !sceneNode->getUseGlobalCursors() ||
		 ( mStyleConfig.WinFlags & UI_WIN_NO_DECORATION ) || !isResizeable() )
		return;

	EventDispatcher* eventDispatcher = sceneNode->getEventDispatcher();

	Vector2i pos = eventDispatcher->getMousePos();

	worldToNode( pos );

	const Node* node = eventDispatcher->getMouseOverNode();

	if ( node == this ) {
		if ( pos.x <= mBorderLeft->getSize().getWidth() ) {
			sceneNode->setCursor( Cursor::SizeNWSE ); // RESIZE_TOPLEFT
		} else if ( pos.x >= ( getSize().getWidth() - mBorderRight->getSize().getWidth() ) ) {
			sceneNode->setCursor( Cursor::SizeNESW ); // RESIZE_TOPRIGHT
		} else if ( pos.y <= mBorderBottom->getSize().getHeight() ) {
			if ( pos.x < mStyleConfig.MinCornerDistance ) {
				sceneNode->setCursor( Cursor::SizeNWSE ); // RESIZE_TOPLEFT
			} else if ( pos.x > getSize().getWidth() - mStyleConfig.MinCornerDistance ) {
				sceneNode->setCursor( Cursor::SizeNESW ); // RESIZE_TOPRIGHT
			} else {
				sceneNode->setCursor( Cursor::SizeNS ); // RESIZE_TOP
			}
		} else if ( !( eventDispatcher->getPressTrigger() & EE_BUTTON_LMASK ) ) {
			sceneNode->setCursor( Cursor::Arrow );
		}
	} else if ( node == mBorderBottom ) {
		if ( pos.x < mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( Cursor::SizeNESW ); // RESIZE_LEFTBOTTOM
		} else if ( pos.x > getSize().getWidth() - mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( Cursor::SizeNWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			sceneNode->setCursor( Cursor::SizeNS ); // RESIZE_BOTTOM
		}
	} else if ( node == mBorderLeft ) {
		if ( pos.y >= getSize().getHeight() - mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( Cursor::SizeNESW ); // RESIZE_LEFTBOTTOM
		} else {
			sceneNode->setCursor( Cursor::SizeWE ); // RESIZE_LEFT
		}
	} else if ( node == mBorderRight ) {
		if ( pos.y >= getSize().getHeight() - mStyleConfig.MinCornerDistance ) {
			sceneNode->setCursor( Cursor::SizeNWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			sceneNode->setCursor( Cursor::SizeWE ); // RESIZE_RIGHT
		}
	}
}

std::string UIWindow::getWindowFlagsString() const {
	std::vector<std::string> flags;
	if ( getWinFlags() & UI_WIN_DEFAULT_FLAGS )
		flags.push_back( "default" );
	if ( getWinFlags() & UI_WIN_CLOSE_BUTTON )
		flags.push_back( "close" );
	if ( getWinFlags() & UI_WIN_MAXIMIZE_BUTTON )
		flags.push_back( "maximize" );
	if ( getWinFlags() & UI_WIN_DRAGABLE_CONTAINER )
		flags.push_back( "draggable" );
	if ( getWinFlags() & UI_WIN_SHADOW )
		flags.push_back( "shadow" );
	if ( getWinFlags() & UI_WIN_MODAL )
		flags.push_back( "modal" );
	if ( getWinFlags() & UI_WIN_NO_DECORATION )
		flags.push_back( "borderless" );
	if ( getWinFlags() & UI_WIN_RESIZEABLE )
		flags.push_back( "resizeable" );
	if ( getWinFlags() & UI_WIN_SHARE_ALPHA_WITH_CHILDS )
		flags.push_back( "shareopacity" );
	if ( getWinFlags() & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS )
		flags.push_back( "buttonactions" );
	if ( getWinFlags() & UI_WIN_FRAME_BUFFER )
		flags.push_back( "framebuffer" );
	if ( getWinFlags() & UI_WIN_COLOR_BUFFER )
		flags.push_back( "colorbuffer" );
	return String::join( flags, '|' );
}

std::string UIWindow::getPropertyString( const PropertyDefinition* propertyDef,
										 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Width:
			return String::fromFloat( getSize().getWidth(), "dp" );
		case PropertyId::Height:
			return String::fromFloat( getSize().getHeight(), "dp" );
		case PropertyId::WindowTitle:
			return getTitle().toUtf8();
		case PropertyId::WindowOpacity:
			return String::toString( getBaseAlpha() / 255.f );
		case PropertyId::WindowButtonsOffset:
			return String::format( "%ddp", mStyleConfig.ButtonsOffset.x ) + " " +
				   String::format( "%ddp", mStyleConfig.ButtonsOffset.y );
		case PropertyId::WindowFlags:
			return getWindowFlagsString();
		case PropertyId::WindowTitlebarSize:
			return String::format( "%ddp", mStyleConfig.TitlebarSize.x ) + " " +
				   String::format( "%ddp", mStyleConfig.TitlebarSize.y );
		case PropertyId::WindowBorderSize:
			return String::format( "%ddp", mStyleConfig.BorderSize.x ) + " " +
				   String::format( "%ddp", mStyleConfig.BorderSize.y );
		case PropertyId::WindowMinSize:
			return String::fromFloat( mStyleConfig.MinWindowSize.x, "dp" ) + " " +
				   String::fromFloat( mStyleConfig.MinWindowSize.y, "dp" );
		case PropertyId::WindowButtonsSeparation:
			return String::format( "%ddp", mStyleConfig.ButtonsSeparation );
		case PropertyId::WindowCornerDistance:
			return String::format( "%ddp", mStyleConfig.MinCornerDistance );
		case PropertyId::WindowTitlebarAutoSize:
			return mStyleConfig.TitlebarAutoSize ? "true" : "false";
		case PropertyId::WindowBorderAutoSize:
			return mStyleConfig.BorderAutoSize ? "true" : "false";
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIWindow::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Width,
				   PropertyId::Height,
				   PropertyId::WindowTitle,
				   PropertyId::WindowOpacity,
				   PropertyId::WindowButtonsOffset,
				   PropertyId::WindowFlags,
				   PropertyId::WindowTitlebarSize,
				   PropertyId::WindowBorderSize,
				   PropertyId::WindowMinSize,
				   PropertyId::WindowButtonsSeparation,
				   PropertyId::WindowCornerDistance,
				   PropertyId::WindowTitlebarAutoSize,
				   PropertyId::WindowBorderAutoSize };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIWindow::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Width:
			setSize( attribute.asDpDimension(), getSize().getHeight() );
			break;
		case PropertyId::Height:
			setSize( getSize().getWidth(), attribute.asDpDimension( this ) );
			break;
		case PropertyId::WindowTitle:
			setTitle( getUISceneNode()->getTranslatorString( attribute.value() ) );
			break;
		case PropertyId::WindowOpacity:
			setWindowOpacity( (Uint8)eemin<Uint32>( (Uint32)attribute.asFloat() * 255.f, 255u ) );
			break;
		case PropertyId::WindowButtonsOffset:
			mStyleConfig.ButtonsOffset = attribute.asDpDimensionVector2i( this );
			fixChildsSize();
			break;
		case PropertyId::WindowFlags: {
			std::string flagsStr = attribute.asString();
			String::toLowerInPlace( flagsStr );
			std::vector<std::string> strings = String::split( flagsStr, '|' );
			Uint32 winflags = 0;

			if ( strings.size() ) {
				for ( std::size_t i = 0; i < strings.size(); i++ ) {
					std::string cur = strings[i];

					if ( "default" == cur )
						winflags |= UI_WIN_DEFAULT_FLAGS;
					else if ( "close" == cur )
						winflags |= UI_WIN_CLOSE_BUTTON;
					else if ( "maximize" == cur )
						winflags |= UI_WIN_MAXIMIZE_BUTTON;
					else if ( "minimize" == cur )
						winflags |= UI_WIN_MINIMIZE_BUTTON;
					else if ( "draggable" == cur )
						winflags |= UI_WIN_DRAGABLE_CONTAINER;
					else if ( "shadow" == cur )
						winflags |= UI_WIN_SHADOW;
					else if ( "modal" == cur )
						winflags |= UI_WIN_MODAL;
					else if ( "noborder" == cur || "borderless" == cur || "undecorated" == cur )
						winflags |= UI_WIN_NO_DECORATION;
					else if ( "resizeable" == cur )
						winflags |= UI_WIN_RESIZEABLE;
					else if ( "shareopacity" == cur )
						winflags |= UI_WIN_SHARE_ALPHA_WITH_CHILDS;
					else if ( "buttonactions" == cur )
						winflags |= UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS;
					else if ( "framebuffer" == cur )
						winflags |= UI_WIN_FRAME_BUFFER;
					else if ( "colorbuffer" == cur )
						winflags |= UI_WIN_COLOR_BUFFER;
				}

				/// TODO: WinFlags should replace old winFlags
				if ( winflags != mStyleConfig.WinFlags ) {
					mStyleConfig.WinFlags |= winflags;
					updateWinFlags();
				}
			}
			break;
		}
		case PropertyId::WindowTitlebarSize:
			mStyleConfig.TitlebarSize = attribute.asDpDimensionSizei( this );
			mStyleConfig.TitlebarAutoSize = false;
			fixChildsSize();
			break;
		case PropertyId::WindowBorderSize:
			mStyleConfig.BorderSize = attribute.asDpDimensionSizei( this );
			mStyleConfig.BorderAutoSize = false;
			fixChildsSize();
			break;
		case PropertyId::WindowMinSize:
			mStyleConfig.MinWindowSize = attribute.asDpDimensionSizef( this );
			fixChildsSize();
			break;
		case PropertyId::WindowButtonsSeparation:
			mStyleConfig.ButtonsSeparation = attribute.asDpDimensionUint( this );
			fixChildsSize();
			break;
		case PropertyId::WindowCornerDistance:
			mStyleConfig.MinCornerDistance = attribute.asDpDimensionI( this );
			break;
		case PropertyId::WindowTitlebarAutoSize:
			mStyleConfig.TitlebarAutoSize = attribute.asBool();
			fixChildsSize();
			break;
		case PropertyId::WindowBorderAutoSize:
			mStyleConfig.BorderAutoSize = attribute.asBool();
			fixChildsSize();
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

void UIWindow::loadFromXmlNode( const pugi::xml_node& node ) {
	UIWidget::loadFromXmlNode( node );

	show();
}

void UIWindow::preDraw() {}

void UIWindow::postDraw() {}

Uint32 UIWindow::onKeyDown( const KeyEvent& event ) {
	std::string cmd = mKeyBindings.getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
	if ( !cmd.empty() ) {
		executeKeyBindingCommand( cmd );
		return 0;
	}
	return UIWidget::onKeyDown( event );
}

KeyBindings& UIWindow::getKeyBindings() {
	return mKeyBindings;
}

void UIWindow::setKeyBindings( const KeyBindings& keyBindings ) {
	mKeyBindings = keyBindings;
}

void UIWindow::addKeyBindingString( const std::string& shortcut, const std::string& command ) {
	mKeyBindings.addKeybindString( shortcut, command );
}

void UIWindow::addKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command ) {
	mKeyBindings.addKeybind( shortcut, command );
}

void UIWindow::replaceKeyBindingString( const std::string& shortcut, const std::string& command ) {
	mKeyBindings.replaceKeybindString( shortcut, command );
}

void UIWindow::replaceKeyBinding( const KeyBindings::Shortcut& shortcut,
								  const std::string& command ) {
	mKeyBindings.replaceKeybind( shortcut, command );
}

void UIWindow::addKeyBindsString( const std::map<std::string, std::string>& binds ) {
	mKeyBindings.addKeybindsString( binds );
}

void UIWindow::addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds ) {
	mKeyBindings.addKeybinds( binds );
}

void UIWindow::setKeyBindingCommand( const std::string& command,
									 UIWindow::KeyBindingCommand func ) {
	mKeyBindingCommands[command] = func;
}

void UIWindow::executeKeyBindingCommand( const std::string& command ) {
	auto cmdIt = mKeyBindingCommands.find( command );
	if ( cmdIt != mKeyBindingCommands.end() ) {
		cmdIt->second();
	}
}

void UIWindow::sendWindowToFront() {
	toFront();
	sendCommonEvent( Event::OnWindowToFront );
}

void UIWindow::checkEphemeralClose() {
	Node* focusNode = getUISceneNode()->getUIEventDispatcher()->getFocusNode();
	if ( !mShowWhenReady && ( mStyleConfig.WinFlags & UI_WIN_EPHEMERAL ) && focusNode != this &&
		 !inParentTreeOf( focusNode ) )
		closeWindow();
}

}} // namespace EE::UI
