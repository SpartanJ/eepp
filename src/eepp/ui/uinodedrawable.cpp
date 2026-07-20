#include <eepp/core/core.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/math/easing.hpp>
#include <eepp/network/http.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>
using namespace EE::Math::easing;

namespace EE { namespace UI {

void UINodeDrawable::repeatFromText( const std::string& text, RepeatX& repeatX, RepeatY& repeatY ) {
	auto parts = String::split( text, ' ' );

	std::string xVal;
	std::string yVal;

	if ( parts.size() == 1 ) {
		if ( "repeat-x" == parts[0] ) {
			xVal = "repeat";
			yVal = "no-repeat";
		} else if ( "repeat-y" == parts[0] ) {
			xVal = "no-repeat";
			yVal = "repeat";
		} else {
			xVal = parts[0];
			yVal = parts[0];
		}
	} else if ( parts.size() >= 2 ) {
		xVal = parts[0];
		yVal = parts[1];
	}

	auto parseOne = []( const std::string& val ) {
		if ( "no-repeat" == val )
			return RepeatX::NoRepeat;
		if ( "space" == val )
			return RepeatX::Space;
		if ( "round" == val )
			return RepeatX::Round;
		return RepeatX::Repeat;
	};

	repeatX = parseOne( xVal );
	repeatY = static_cast<RepeatY>( parseOne( yVal ) );
}

UINodeDrawable::LayerDrawable::Origin
UINodeDrawable::LayerDrawable::originFromText( const std::string& text ) {
	if ( "border-box" == text )
		return Origin::BorderBox;
	if ( "content-box" == text )
		return Origin::ContentBox;
	return Origin::PaddingBox;
}

UINodeDrawable::LayerDrawable::Clip
UINodeDrawable::LayerDrawable::clipFromText( const std::string& text ) {
	if ( "padding-box" == text )
		return Clip::PaddingBox;
	if ( "content-box" == text )
		return Clip::ContentBox;
	return Clip::BorderBox;
}

UINodeDrawable::LayerDrawable::Attachment
UINodeDrawable::LayerDrawable::attachmentFromText( const std::string& text ) {
	if ( "fixed" == text )
		return Attachment::Fixed;
	if ( "local" == text )
		return Attachment::Local;
	return Attachment::Scroll;
}

UINodeDrawable* UINodeDrawable::New( UINode* owner ) {
	return eeNew( UINodeDrawable, ( owner ) );
}

UINodeDrawable::UINodeDrawable( UINode* owner ) :
	Drawable( Drawable::UINODEDRAWABLE ),
	mOwner( owner ),
	mBackgroundColor( owner ),
	mNeedsUpdate( true ),
	mClipEnabled( false ),
	mSmooth( false ) {
	mBackgroundColor.setColor( Color::Transparent );
}

UINodeDrawable::~UINodeDrawable() {
	clearDrawables();
}

void UINodeDrawable::clearDrawables() {
	for ( auto& drawable : mGroup ) {
		eeDelete( drawable.second );
	}

	mGroup.clear();
	mBackgroundColor.setColor( Color::Transparent );
}

void UINodeDrawable::setBorderRadius( const Uint32& radius ) {
	mBackgroundColor.setRadius( radius );
}

Uint32 UINodeDrawable::getBorderRadius() const {
	return mBackgroundColor.getRadius();
}

bool UINodeDrawable::layerExists( int index ) {
	return mGroup.find( index ) != mGroup.end();
}

UINodeDrawable::LayerDrawable* UINodeDrawable::getLayer( int index ) {
	auto it = mGroup.find( index );

	if ( it == mGroup.end() ) {
		mGroup[index] = UINodeDrawable::LayerDrawable::New( this );

		// HTML background-repeat defaults to "repeat", non-HTML to
		// "no-repeat". The LayerDrawable constructor uses NoRepeat
		// (the eepp/non-HTML default), so reset it for Html mode.
		if ( mBackgroundMode == BackgroundMode::Html ) {
			auto* layer = mGroup[index];
			layer->setRepeatX( RepeatX::Repeat );
			layer->setRepeatY( RepeatY::Repeat );
		}
	}

	return mGroup[index];
}

void UINodeDrawable::setDrawable( int index, Drawable* drawable, bool ownIt ) {
	if ( drawable != getLayer( index )->getDrawable() ) {
		getLayer( index )->setDrawable( drawable, ownIt );
	}
}

void UINodeDrawable::setDrawable( int index, const std::string& drawable ) {
	if ( drawable != getLayer( index )->getDrawableRef() ) {
		getLayer( index )->setDrawable( drawable );
	}
}

void UINodeDrawable::setDrawablePositionX( int index, const std::string& positionX ) {
	getLayer( index )->setPositionX( positionX );
}

void UINodeDrawable::setDrawablePositionY( int index, const std::string& positionY ) {
	getLayer( index )->setPositionY( positionY );
}

void UINodeDrawable::setDrawableRepeat( int index, const std::string& repeatRule ) {
	RepeatX rx;
	RepeatY ry;
	repeatFromText( repeatRule, rx, ry );
	getLayer( index )->setRepeatX( rx );
	getLayer( index )->setRepeatY( ry );

	for ( auto& layIt : mGroup ) {
		if ( layIt.second->getRepeatX() != RepeatX::NoRepeat ||
			 layIt.second->getRepeatY() != RepeatY::NoRepeat ) {
			setClipEnabled( true );
			break;
		}
	}
}

void UINodeDrawable::setDrawableSize( int index, const std::string& sizeEq ) {
	getLayer( index )->setSizeEq( sizeEq );
}

void UINodeDrawable::setDrawableOrigin( int index, const std::string& origin ) {
	getLayer( index )->setOrigin( origin );
}

void UINodeDrawable::setDrawableClip( int index, const std::string& clip ) {
	getLayer( index )->setClip( clip );
}

void UINodeDrawable::setDrawableAttachment( int index, const std::string& attachment ) {
	getLayer( index )->setAttachment( attachment );
}

void UINodeDrawable::setDrawableColor( int index, const Color& color ) {
	getLayer( index )->setColor( color );
}

void UINodeDrawable::setBackgroundColor( const Color& color ) {
	mBackgroundColor.setColor( color );
}

Color UINodeDrawable::getBackgroundColor() const {
	return mBackgroundColor.getColor();
}

bool UINodeDrawable::getClipEnabled() const {
	return mClipEnabled;
}

void UINodeDrawable::setClipEnabled( bool clipEnabled ) {
	mClipEnabled = clipEnabled;
}

void UINodeDrawable::invalidate() {
	mNeedsUpdate = true;
	mBackgroundColor.invalidate();
}

UINode* UINodeDrawable::getOwner() const {
	return mOwner;
}

UIBackgroundDrawable& UINodeDrawable::getBackgroundDrawable() {
	return mBackgroundColor;
}

bool UINodeDrawable::hasDrawableLayers() const {
	return !mGroup.empty();
}

bool UINodeDrawable::isSmooth() const {
	return mSmooth;
}

void UINodeDrawable::setSmooth( bool smooth ) {
	mSmooth = smooth;
	getBackgroundDrawable().setSmooth( smooth );
}

void UINodeDrawable::setBackgroundMode( BackgroundMode mode ) {
	mBackgroundMode = mode;

	// HTML background-repeat defaults to "repeat", non-HTML defaults to
	// "no-repeat". When switching mode, update any existing layers that
	// still carry the LayerDrawable default (NoRepeat for both axes).
	if ( mode == BackgroundMode::Html ) {
		for ( auto& entry : mGroup ) {
			auto* layer = entry.second;
			if ( layer->getRepeatX() == RepeatX::NoRepeat &&
				 layer->getRepeatY() == RepeatY::NoRepeat ) {
				layer->setRepeatX( RepeatX::Repeat );
				layer->setRepeatY( RepeatY::Repeat );
			}
		}
	}
}

BackgroundMode UINodeDrawable::getBackgroundMode() const {
	return mBackgroundMode;
}

Sizef UINodeDrawable::getSize() {
	return mSize;
}

Sizef UINodeDrawable::getPixelsSize() {
	return mSize;
}

void UINodeDrawable::setSize( const Sizef& size ) {
	if ( size != mSize ) {
		mSize = size;
		onSizeChange();
	}
}

void UINodeDrawable::draw( const Vector2f& position, const Sizef& size ) {
	draw( position, size, 255 );
}

void UINodeDrawable::draw( const Vector2f& position, const Sizef& size, const Uint32& alpha ) {
	if ( position != mPosition ) {
		mPosition = position;
		invalidate();
	}

	if ( size != mSize ) {
		mSize = size;
		invalidate();
	}

	if ( mNeedsUpdate )
		update();

	if ( mClipEnabled || mBackgroundMode == BackgroundMode::Html )
		GLi->getClippingMask()->clipPlaneEnable( mPosition.x, mPosition.y, mSize.x, mSize.y );

	if ( mBackgroundColor.getColor().a != 0 ) {
		if ( alpha != 255 ) {
			Color color = mBackgroundColor.getColor();
			mBackgroundColor.setAlpha( alpha * color.a / 255 );
			mBackgroundColor.draw( position, size );
			mBackgroundColor.setAlpha( color.a );
		} else {
			mBackgroundColor.draw( position, size );
		}
	}

	bool masked = false;
	ClippingMask* clippingMask = GLi->getClippingMask();
	if ( !mGroup.empty() && mBackgroundColor.hasRadius() ) {
		masked = true;
		clippingMask->setMaskMode( ClippingMask::Inclusive );
		clippingMask->clearMasks();
		clippingMask->appendMask( mBackgroundColor );
		clippingMask->stencilMaskEnable();
	}

	bool isPolySmooth = GLi->isPolygonSmooth();
	bool isLineSmooth = GLi->isLineSmooth();

	if ( mSmooth ) {
		if ( !isPolySmooth )
			GLi->polygonSmooth( true );

		if ( !isLineSmooth )
			GLi->lineSmooth( true );
	}

	// Draw in reverse order to respect the background-image specification:
	// "The background images are drawn on stacking context layers on top of each other. The first
	// layer specified is drawn as if it is closest to the user."
	for ( auto drawableIt = mGroup.rbegin(); drawableIt != mGroup.rend(); ++drawableIt ) {
		UINodeDrawable::LayerDrawable* drawable = drawableIt->second;

		bool clipContent = mBackgroundMode == BackgroundMode::Html &&
						   drawable->getClip() == LayerDrawable::Clip::ContentBox;
		Rectf contentRect;
		if ( clipContent && mOwner->isWidget() ) {
			Rectf pad = mOwner->asType<UIWidget>()->getPixelsPadding();
			contentRect.Left = mPosition.x + pad.Left;
			contentRect.Top = mPosition.y + pad.Top;
			contentRect.Right = mPosition.x + mSize.x - pad.Right;
			contentRect.Bottom = mPosition.y + mSize.y - pad.Bottom;
			GLi->getClippingMask()->clipPlaneEnable( contentRect.Left, contentRect.Top,
													 contentRect.Right - contentRect.Left,
													 contentRect.Bottom - contentRect.Top );
		}

		if ( alpha != 255 ) {
			Color color = drawable->getColor();
			drawable->setAlpha( alpha * color.a / 255 );
			drawable->draw( position, size );
			drawable->setAlpha( color.a );
		} else {
			drawable->draw( position, size );
		}

		if ( clipContent )
			GLi->getClippingMask()->clipPlaneDisable();
	}

	if ( mSmooth ) {
		if ( !isPolySmooth )
			GLi->polygonSmooth( isPolySmooth );

		if ( !isLineSmooth )
			GLi->lineSmooth( isLineSmooth );
	}

	if ( masked ) {
		clippingMask->stencilMaskDisable();
	}

	if ( mClipEnabled || mBackgroundMode == BackgroundMode::Html )
		GLi->getClippingMask()->clipPlaneDisable();
}

void UINodeDrawable::draw( const Vector2f& position ) {
	draw( position, mSize );
}

void UINodeDrawable::draw() {
	draw( mPosition, mSize );
}

void UINodeDrawable::onPositionChange() {
	invalidate();
}

void UINodeDrawable::onSizeChange() {
	invalidate();
}

void UINodeDrawable::update() {
	mBackgroundColor.setPosition( mPosition );
	mBackgroundColor.setSize( mSize );
	mNeedsUpdate = false;
}

/**** UINodeDrawable::LayerDrawable ****/

UINodeDrawable::LayerDrawable* UINodeDrawable::LayerDrawable::New( UINodeDrawable* container ) {
	return eeNew( UINodeDrawable::LayerDrawable, ( container ) );
}

UINodeDrawable::LayerDrawable::LayerDrawable( UINodeDrawable* container ) :
	Drawable( UINODEDRAWABLE_LAYERDRAWABLE ),
	mContainer( container ),
	mPositionX( "0px" ),
	mPositionY( "0px" ),
	mSizeEq( "auto" ),
	mNeedsUpdate( false ),
	mOwnsDrawable( false ),
	mDrawable( NULL ),
	mResourceChangeCbId( 0 ),
	mRepeatX( RepeatX::NoRepeat ),
	mRepeatY( RepeatY::NoRepeat ),
	mOriginEq( "padding-box" ),
	mClipEq( "border-box" ),
	mAttachmentEq( "scroll" ),
	mOrigin( Origin::PaddingBox ),
	mClip( Clip::BorderBox ),
	mAttachment( Attachment::Scroll ),
	mAsyncDrawableAlive( std::make_shared<std::atomic<bool>>( true ) ) {}

UINodeDrawable::LayerDrawable::~LayerDrawable() {
	if ( mAsyncDrawableAlive )
		mAsyncDrawableAlive->store( false, std::memory_order_release );

	if ( NULL != mDrawable && 0 != mResourceChangeCbId && mDrawable->isDrawableResource() ) {
		reinterpret_cast<DrawableResource*>( mDrawable )
			->popResourceChangeCallback( mResourceChangeCbId );
	}

	if ( mOwnsDrawable )
		eeSAFE_DELETE( mDrawable );
}

void UINodeDrawable::LayerDrawable::draw() {
	draw( mPosition, mSize );
}

void UINodeDrawable::LayerDrawable::draw( const Vector2f& position ) {
	draw( position, mSize );
}

static void repeatYdraw( Drawable* drawable, const Vector2f& position, const Vector2f& offset,
						 const Sizef& size, const Sizef& drawableSize ) {
	if ( drawableSize.getHeight() > 0 ) {
		Float startY = position.y + offset.y - drawableSize.getHeight();
		while ( startY > position.y - drawableSize.getHeight() ) {
			drawable->draw( Vector2f( position.x + offset.x, startY ), drawableSize );
			startY -= drawableSize.getHeight();
		};
		drawable->draw( position + offset, drawableSize );
		startY = position.y + offset.y + drawableSize.getHeight();
		while ( startY < position.y + size.getHeight() ) {
			drawable->draw( Vector2f( position.x + offset.x, startY ), drawableSize );
			startY += drawableSize.getHeight();
		};
	}
}

void UINodeDrawable::LayerDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( position != mPosition ) {
		mPosition = position;
		invalidate();
	}

	if ( size != mSize ) {
		mSize = size;
		invalidate();
	}

	if ( mDrawable == NULL )
		return;

	if ( mNeedsUpdate )
		update();

	RGB prevColor = mDrawable->getColorFilter();
	if ( mColorWasSet )
		mDrawable->setColorFilter( getColor() );
	mDrawable->setAlpha( getAlpha() );

	Vector2f effectivePos = mPosition;
	if ( mAttachment == Attachment::Fixed ) {
		auto* sceneNode = mContainer->getOwner()->getUISceneNode();
		if ( sceneNode )
			effectivePos = sceneNode->getPosition();
	}

	auto drawY = [this, &effectivePos]( Float xPos, Float drawH, Sizef tileSz ) {
		switch ( mRepeatY ) {
			case RepeatY::NoRepeat:
				mDrawable->draw( Vector2f( xPos, effectivePos.y + mOffset.y ), tileSz );
				break;
			case RepeatY::Repeat:
				repeatYdraw( mDrawable, effectivePos, Vector2f( xPos - effectivePos.x, mOffset.y ),
							 mSize, tileSz );
				break;
			case RepeatY::Space: {
				if ( drawH <= 0 )
					break;
				int count = (int)( mSize.getHeight() / drawH );
				if ( count < 1 )
					count = 1;
				Float totalTilesH = count * drawH;
				Float gap = ( mSize.getHeight() - totalTilesH ) / (Float)( count + 1 );
				for ( int i = 0; i < count; ++i ) {
					Float y = effectivePos.y + gap + (Float)i * ( drawH + gap );
					mDrawable->draw( Vector2f( xPos, y ), tileSz );
				}
				break;
			}
			case RepeatY::Round: {
				if ( drawH <= 0 )
					break;
				Float scale = mSize.getHeight() / drawH;
				int count = (int)Math::round( scale );
				if ( count < 1 )
					count = 1;
				Float roundedH = mSize.getHeight() / (Float)count;
				Float aspect = drawH > 0 ? tileSz.getWidth() / tileSz.getHeight() : 1.0f;
				Sizef roundSz( roundedH * aspect, roundedH );
				for ( int i = 0; i < count; ++i ) {
					Float y = effectivePos.y + (Float)i * roundedH;
					mDrawable->draw( Vector2f( xPos, y ), roundSz );
				}
				break;
			}
			default:
				break;
		}
	};

	switch ( mRepeatX ) {
		case RepeatX::NoRepeat:
			drawY( effectivePos.x + mOffset.x, mDrawableSize.getHeight(), mDrawableSize );
			break;
		case RepeatX::Repeat: {
			if ( mDrawableSize.getWidth() > 0 ) {
				Float startX = effectivePos.x + mOffset.x - mDrawableSize.getWidth();
				while ( startX > effectivePos.x - mDrawableSize.getWidth() ) {
					drawY( startX, mDrawableSize.getHeight(), mDrawableSize );
					startX -= mDrawableSize.getWidth();
				}
				drawY( effectivePos.x + mOffset.x, mDrawableSize.getHeight(), mDrawableSize );
				startX = effectivePos.x + mOffset.x + mDrawableSize.getWidth();
				while ( startX < effectivePos.x + mSize.getWidth() ) {
					drawY( startX, mDrawableSize.getHeight(), mDrawableSize );
					startX += mDrawableSize.getWidth();
				}
			}
			break;
		}
		case RepeatX::Space: {
			if ( mDrawableSize.getWidth() <= 0 )
				break;
			int count = (int)( mSize.getWidth() / mDrawableSize.getWidth() );
			if ( count < 1 )
				count = 1;
			Float totalTilesW = count * mDrawableSize.getWidth();
			Float gap = ( mSize.getWidth() - totalTilesW ) / (Float)( count + 1 );
			for ( int i = 0; i < count; ++i ) {
				Float x = effectivePos.x + gap + (Float)i * ( mDrawableSize.getWidth() + gap );
				drawY( x, mDrawableSize.getHeight(), mDrawableSize );
			}
			break;
		}
		case RepeatX::Round: {
			if ( mDrawableSize.getWidth() <= 0 )
				break;
			Float scale = mSize.getWidth() / mDrawableSize.getWidth();
			int count = (int)Math::round( scale );
			if ( count < 1 )
				count = 1;
			Float roundedW = mSize.getWidth() / (Float)count;
			Float aspect = mDrawableSize.getWidth() > 0
							   ? mDrawableSize.getHeight() / mDrawableSize.getWidth()
							   : 1.0f;
			Float roundedH = roundedW * aspect;
			Sizef roundSz( roundedW, roundedH );
			for ( int i = 0; i < count; ++i ) {
				Float x = effectivePos.x + (Float)i * roundedW;
				drawY( x, roundedH, roundSz );
			}
			break;
		}
		default:
			break;
	}
	if ( mColorWasSet )
		mDrawable->setColorFilter( prevColor );
}

Sizef UINodeDrawable::LayerDrawable::getSize() {
	return mSize;
}

Sizef UINodeDrawable::LayerDrawable::getPixelsSize() {
	return mSize;
}

void UINodeDrawable::LayerDrawable::setSize( const Sizef& size ) {
	if ( size != mSize ) {
		mSize = size;
		invalidate();
	}
}

Drawable* UINodeDrawable::LayerDrawable::getDrawable() const {
	return mDrawable;
}

const std::string& UINodeDrawable::LayerDrawable::getDrawableRef() const {
	return mDrawableRef;
}

void UINodeDrawable::LayerDrawable::setDrawable( Drawable* drawable, const bool& ownIt ) {
	if ( drawable == mDrawable )
		return;

	if ( NULL != mDrawable ) {
		if ( mDrawable->isDrawableResource() ) {
			reinterpret_cast<DrawableResource*>( mDrawable )
				->popResourceChangeCallback( mResourceChangeCbId );
		}

		if ( mOwnsDrawable ) {
			eeSAFE_DELETE( mDrawable );
		}
	}
	mTexture.reset();

	mDrawable = drawable;
	mDrawableRef = "";
	mOwnsDrawable = ownIt;
	invalidate();

	if ( NULL != mDrawable && mDrawable->isDrawableResource() ) {
		mResourceChangeCbId =
			reinterpret_cast<DrawableResource*>( mDrawable )
				->pushResourceChangeCallback(
					[this]( Uint32, DrawableResource::Event event, DrawableResource* ) {
						invalidate();
						if ( event == DrawableResource::Event::Unload ) {
							mResourceChangeCbId = 0;
							mDrawable = NULL;
							mOwnsDrawable = false;
						}
					} );
	}
}

void UINodeDrawable::LayerDrawable::setDrawable( TexturePtr texture ) {
	Texture* drawable = texture.get();
	setDrawable( drawable, false );
	mTexture = std::move( texture );
}

void UINodeDrawable::LayerDrawable::setDrawable( const std::string& drawableRef ) {
	if ( drawableRef == "none" ) {
		setDrawable( nullptr, false );
		return;
	}

	if ( loadRemoteDrawable( drawableRef ) ) {
		mDrawableRef = drawableRef;
		return;
	}

	bool ownIt;
	Drawable* drawable = createDrawable( drawableRef, mSize, ownIt );

	setDrawable( drawable, ownIt );

	mDrawableRef = drawableRef;
}

bool UINodeDrawable::LayerDrawable::loadRemoteDrawable( const std::string& value ) {
	FunctionString functionType = FunctionString::parse( value );
	std::string path;

	if ( !functionType.isEmpty() ) {
		if ( functionType.getName() != "url" || functionType.getParameters().empty() )
			return false;
		path = functionType.getParameters().at( 0 );
	} else {
		path = value;
	}

	if ( !path.empty() && path.front() == '\'' && path.back() == '\'' )
		String::trimInPlace( path, '\'' );
	else if ( !path.empty() && path.front() == '"' && path.back() == '"' )
		String::trimInPlace( path, '"' );

	UINode* owner = mContainer ? mContainer->getOwner() : nullptr;
	UISceneNode* scene = owner ? owner->getUISceneNode() : nullptr;
	if ( !scene )
		return false;

	URI uri( path );
	if ( uri.getScheme().empty() )
		uri = scene->solveRelativePath( uri );

	if ( uri.getScheme() != "http" && uri.getScheme() != "https" )
		return false;

	if ( value == mDrawableRef && mDrawable != nullptr )
		return true;

	std::string url = uri.toString();
	ResourceScopePtr resourceScope = scene->getResourceScope();
	if ( TexturePtr texture = resourceScope->findTexture( url ) ) {
		if ( mDrawable != texture.get() ) {
			++mRemoteDrawableLoadId;
			setDrawable( std::move( texture ) );
		}
		return true;
	}

	auto resourceState = scene->getAsyncResourceLoadState();
	Uint64 resourceGeneration =
		resourceState ? resourceState->generation.load( std::memory_order_acquire ) : 0;
	Uint64 loadId = ++mRemoteDrawableLoadId;
	auto alive = mAsyncDrawableAlive;
	TexturePtr texture = TextureFactory::instance()->createEmptyTexture(
		1, 1, 4, Color::Transparent, false, Texture::ClampMode::ClampToEdge, false, false, url );
	if ( texture ) {
		resourceScope->publishLocal( url, texture );
		setDrawable( texture );
	}

	Http::Request::FieldTable headers;
	if ( !scene->getReferer().empty() )
		headers["referer"] = scene->getReferer().toString();
	Http::getAsync(
		[resourceState, resourceGeneration, alive, loadId, texture, url = std::move( url ),
		 this]( const Http&, Http::Request&, Http::Response& response ) {
			if ( !UISceneNode::isAsyncResourceLoadCurrent( resourceState, resourceGeneration ) ||
				 !alive || !alive->load( std::memory_order_acquire ) || texture == nullptr )
				return;

			if ( response.isOK() && !response.getBody().empty() ) {
				std::string imageData( response.getBody() );
				UISceneNode::runAsyncResourceOnMainThread(
					resourceState, resourceGeneration,
					[alive, loadId, texture, imageData = std::move( imageData ),
					 this]( UISceneNode* ) mutable {
						if ( !alive || !alive->load( std::memory_order_acquire ) ||
							 loadId != mRemoteDrawableLoadId || mDrawable != texture.get() )
							return;

						Image image( reinterpret_cast<const Uint8*>( imageData.data() ),
									 imageData.size() );
						if ( image.getPixels() != nullptr )
							texture->replace( &image );
					} );
			} else {
				Log::debug( "UINodeDrawable::LayerDrawable::loadRemoteDrawable: could not "
							"download image: %s. Error: %d\n%s",
							url, response.getStatus(), response.getBody() );
			}
		},
		uri, Seconds( 5 ), {}, headers, "", true, Http::getEnvProxyURI() );

	return true;
}

Drawable* UINodeDrawable::LayerDrawable::createDrawable( const std::string& value,
														 const Sizef& size, bool& ownIt ) {
	return CSS::StyleSheetSpecification::instance()->getDrawableImageParser().createDrawable(
		value, size, ownIt, mContainer->getOwner() );
}

const Vector2f& UINodeDrawable::LayerDrawable::getOffset() const {
	if ( mNeedsUpdate )
		const_cast<LayerDrawable*>( this )->update();

	return mOffset;
}

UINodeDrawable::RepeatX UINodeDrawable::LayerDrawable::getRepeatX() const {
	return mRepeatX;
}

UINodeDrawable::RepeatY UINodeDrawable::LayerDrawable::getRepeatY() const {
	return mRepeatY;
}

void UINodeDrawable::LayerDrawable::setRepeatX( RepeatX repeatX ) {
	if ( mRepeatX != repeatX ) {
		mRepeatX = repeatX;
		invalidate();
	}
}

void UINodeDrawable::LayerDrawable::setRepeatY( RepeatY repeatY ) {
	if ( mRepeatY != repeatY ) {
		mRepeatY = repeatY;
		invalidate();
	}
}

void UINodeDrawable::LayerDrawable::invalidate() {
	mNeedsUpdate = true;
}

const Sizef& UINodeDrawable::LayerDrawable::getDrawableSize() const {
	if ( mNeedsUpdate )
		const_cast<LayerDrawable*>( this )->update();

	return mDrawableSize;
}

void UINodeDrawable::LayerDrawable::setDrawableSize( const Sizef& drawableSize ) {
	mDrawableSize = drawableSize;
}

Sizef UINodeDrawable::LayerDrawable::calcDrawableSize( const std::string& drawableSizeEq ) {
	Sizef size;

	if ( NULL == mDrawable )
		return Sizef::Zero;

	if ( drawableSizeEq == "auto" ) {
		if ( mDrawable->getDrawableType() == Drawable::RECTANGLE ||
			 mDrawable->getDrawableType() == LINEARGRADIENT ||
			 mDrawable->getDrawableType() == REPEATINGLINEARGRADIENT ||
			 mDrawable->getDrawableType() == RADIALGRADIENT ||
			 mDrawable->getDrawableType() == REPEATINGRADIALGRADIENT ) {
			size = mSize;
		} else {
			size = mDrawable->getPixelsSize();
			// For Html mode, mOffset (from calcPosition) is in CSS‑physical pixels
			// (scaled by PixelDensity via convertLength -> dpToPx), but
			// getPixelsSize() returns raw image pixels.  Scale the intrinsic
			// size so both values use the same coordinate system, otherwise
			// the visible sprite‑atlas region drifts when PixelDensity != 1.
			if ( mContainer->getBackgroundMode() == BackgroundMode::Html ) {
				size = Sizef( size.x * PixelDensity::getPixelDensity(),
							  size.y * PixelDensity::getPixelDensity() );
			}
		}
	} else if ( drawableSizeEq == "expand" ) {
		size = mSize;
	} else if ( drawableSizeEq == "contain" ) {
		Sizef drawableSize( mDrawable->getSize() );
		Float Scale1 = mSize.getWidth() / drawableSize.getWidth();
		Float Scale2 = mSize.getHeight() / drawableSize.getHeight();
		Scale1 = eemin( Scale1, Scale2 );
		size = Sizef( drawableSize.getWidth() * Scale1, drawableSize.getHeight() * Scale1 );
	} else if ( drawableSizeEq == "cover" ) {
		Sizef drawableSize( mDrawable->getSize() );
		Float Scale1 = mSize.getWidth() / drawableSize.getWidth();
		Float Scale2 = mSize.getHeight() / drawableSize.getHeight();
		Scale1 = eemax( Scale1, Scale2 );
		size = Sizef( drawableSize.getWidth() * Scale1, drawableSize.getHeight() * Scale1 );
	} else {
		std::vector<std::string> sizePart = String::split( drawableSizeEq, ' ' );

		if ( sizePart.size() == 1 ) {
			sizePart.push_back( "auto" );
		}

		if ( sizePart.size() == 2 ) {
			if ( sizePart[0] == "auto" && sizePart[1] == "auto" ) {
				if ( mDrawable->getDrawableType() == Drawable::RECTANGLE ||
					 mDrawable->getDrawableType() == Drawable::LINEARGRADIENT ||
					 mDrawable->getDrawableType() == Drawable::REPEATINGLINEARGRADIENT ||
					 mDrawable->getDrawableType() == Drawable::RADIALGRADIENT ||
					 mDrawable->getDrawableType() == Drawable::REPEATINGRADIALGRADIENT ) {
					size = mSize;
				} else {
					size = mDrawable->getSize();
				}
			} else if ( sizePart[0] != "auto" ) {
				CSS::StyleSheetLength wl( sizePart[0] );
				size.x = mContainer->getOwner()->convertLength(
					wl, mContainer->getOwner()->getPixelsSize().getWidth() );

				if ( sizePart[1] == "auto" ) {
					Sizef drawableSize( mDrawable->getSize() );
					size.y =
						drawableSize.getHeight() * ( size.getWidth() / drawableSize.getWidth() );
				} else {
					CSS::StyleSheetLength hl( sizePart[1] );
					size.y = mContainer->getOwner()->convertLength(
						hl, mContainer->getOwner()->getPixelsSize().getHeight() );
				}
			} else {
				CSS::StyleSheetLength hl( sizePart[1] );
				size.y = mContainer->getOwner()->convertLength(
					hl, mContainer->getOwner()->getPixelsSize().getHeight() );

				Sizef drawableSize( mDrawable->getSize() );
				size.x = drawableSize.getWidth() * ( size.getHeight() / drawableSize.getHeight() );
			}
		}
	}

	return size;
}

Vector2f UINodeDrawable::LayerDrawable::calcPosition( std::string positionXEq,
													  std::string positionYEq ) {
	Vector2f position( Vector2f::Zero );

	if ( positionXEq.empty() )
		positionXEq = "center";

	if ( positionYEq.empty() )
		positionYEq = "center";

	auto posX = String::split( positionXEq, ' ' );
	auto posY = String::split( positionYEq, ' ' );

	if ( posX.empty() || posY.empty() )
		return position;

	bool needsRoundingX = positionXEq.back() == '%';
	bool needsRoundingY = positionYEq.back() == '%';

	Sizef ownerSize = mContainer->getOwner()->getPixelsSize();
	Float refWidth = ownerSize.getWidth();
	Float refHeight = ownerSize.getHeight();
	Float originOffX = 0;
	Float originOffY = 0;

	if ( mContainer->getBackgroundMode() == BackgroundMode::Html &&
		 mOrigin == Origin::ContentBox ) {
		Rectf pad = mContainer->getOwner()->getPixelsPadding();
		refWidth -= pad.Left + pad.Right;
		refHeight -= pad.Top + pad.Bottom;
		originOffX = pad.Left;
		originOffY = pad.Top;
	}

	if ( posX.size() == 2 ) {
		CSS::StyleSheetLength xl1( posX[0] );
		CSS::StyleSheetLength xl2( posX[1] );

		position.x =
			mContainer->getOwner()->convertLength( xl1, refWidth - mDrawableSize.getWidth() );

		Float xl2Val =
			mContainer->getOwner()->convertLength( xl2, refWidth - mDrawableSize.getWidth() );

		position.x += ( posX[0] == "right" ) ? -xl2Val : xl2Val;
	} else {
		CSS::StyleSheetLength xl( posX[0] );
		position.x =
			mContainer->getOwner()->convertLength( xl, refWidth - mDrawableSize.getWidth() );
	}

	if ( posY.size() == 2 ) {
		CSS::StyleSheetLength yl1( posY[0] );
		CSS::StyleSheetLength yl2( posY[1] );

		position.y =
			mContainer->getOwner()->convertLength( yl1, refHeight - mDrawableSize.getHeight() );

		Float xl2Val =
			mContainer->getOwner()->convertLength( yl2, refHeight - mDrawableSize.getHeight() );

		position.y += ( posY[0] == "bottom" ) ? -xl2Val : xl2Val;
	} else {
		CSS::StyleSheetLength yl( posY[0] );
		position.y =
			mContainer->getOwner()->convertLength( yl, refHeight - mDrawableSize.getHeight() );
	}

	position.x += originOffX;
	position.y += originOffY;

	if ( needsRoundingX )
		position.x = Math::round( position.x );

	if ( needsRoundingY )
		position.y = Math::round( position.y );

	return position;
}

const std::string& UINodeDrawable::LayerDrawable::getSizeEq() const {
	return mSizeEq;
}

void UINodeDrawable::LayerDrawable::setSizeEq( const std::string& sizeEq ) {
	if ( mSizeEq != sizeEq ) {
		mSizeEq = sizeEq;
		invalidate();
	}
}

void UINodeDrawable::LayerDrawable::setOrigin( const std::string& origin ) {
	if ( mOriginEq != origin ) {
		mOriginEq = origin;
		mOrigin = originFromText( origin );
		invalidate();
	}
}

void UINodeDrawable::LayerDrawable::setClip( const std::string& clip ) {
	if ( mClipEq != clip ) {
		mClipEq = clip;
		mClip = clipFromText( clip );
		invalidate();
	}
}

void UINodeDrawable::LayerDrawable::setAttachment( const std::string& attachment ) {
	if ( mAttachmentEq != attachment ) {
		mAttachmentEq = attachment;
		mAttachment = attachmentFromText( attachment );
		invalidate();
	}
}

UINodeDrawable::LayerDrawable::Origin UINodeDrawable::LayerDrawable::getOrigin() const {
	return mOrigin;
}

UINodeDrawable::LayerDrawable::Clip UINodeDrawable::LayerDrawable::getClip() const {
	return mClip;
}

UINodeDrawable::LayerDrawable::Attachment UINodeDrawable::LayerDrawable::getAttachment() const {
	return mAttachment;
}

const std::string& UINodeDrawable::LayerDrawable::getOriginEq() const {
	return mOriginEq;
}

const std::string& UINodeDrawable::LayerDrawable::getClipEq() const {
	return mClipEq;
}

const std::string& UINodeDrawable::LayerDrawable::getAttachmentEq() const {
	return mAttachmentEq;
}

const std::string& UINodeDrawable::LayerDrawable::getPositionY() const {
	return mPositionY;
}

void UINodeDrawable::LayerDrawable::setPositionY( const std::string& positionY ) {
	if ( mPositionY != positionY ) {
		mPositionY = positionY;
		invalidate();
	}
}

const std::string& UINodeDrawable::LayerDrawable::getPositionX() const {
	return mPositionX;
}

void UINodeDrawable::LayerDrawable::setPositionX( const std::string& positionX ) {
	if ( mPositionX != positionX ) {
		mPositionX = positionX;
		invalidate();
	}
}

void UINodeDrawable::LayerDrawable::setOffset( const Vector2f& offset ) {
	mOffset = offset;
}

std::string UINodeDrawable::LayerDrawable::getOffsetEq() {
	return String::fromFloat( mOffset.x, "px" ) + " " + String::fromFloat( mOffset.y, "px" );
}

void UINodeDrawable::LayerDrawable::onPositionChange() {
	invalidate();
}

void UINodeDrawable::LayerDrawable::onColorFilterChange() {
	mColorWasSet = true;
	invalidate();
}

void UINodeDrawable::LayerDrawable::update() {
	if ( mDrawable == NULL )
		return;

	if ( !mDrawableRef.empty() ) {
		setDrawable( mDrawableRef );
	}

	mDrawableSize = calcDrawableSize( mSizeEq );
	mOffset = calcPosition( mPositionX, mPositionY );

	mNeedsUpdate = false;
}

}} // namespace EE::UI
