#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturedrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/network/http.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/drawableimageparser.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>

#include <mutex>

namespace EE { namespace UI {

namespace {

std::string getTextureCacheName( const Network::URI& uri ) {
	std::string filePath( uri.toString() );
	if ( String::startsWith( filePath, "file://" ) )
		filePath = filePath.substr( 7 );
	else
		filePath = uri.getFSPath();

#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( filePath.size() >= 3 && filePath[0] == '/' && String::isLetter( filePath[1] ) &&
		 filePath[2] == ':' ) {
		filePath = filePath.substr( 1 );
	}
#endif

	FileSystem::filePathRemoveProcessPath( filePath );
	return filePath;
}

TexturePtr loadFileTextureCached( const ResourceScopePtr& scope, const std::string& filePath,
								  const std::string& cacheName ) {
	static std::mutex loadMutex;
	std::lock_guard<std::mutex> lock( loadMutex );

	if ( TexturePtr texture = scope->findTexture( cacheName ) )
		return texture;

	TexturePtr texture = TextureFactory::instance()->loadFromFile(
		filePath, false, Texture::ClampMode::ClampToEdge, false, false );
	if ( texture )
		scope->publishLocal( cacheName, texture );
	return texture;
}

} // namespace

UIImage* UIImage::New() {
	return eeNew( UIImage, () );
}

UIImage* UIImage::NewWithTag( const std::string& tag ) {
	return eeNew( UIImage, ( tag ) );
}

UIImage::UIImage( const std::string& tag ) :
	UIWidget( tag ),
	mScaleType( UIScaleType::None ),
	mColor(),
	mAlignOffset( 0, 0 ),
	mAsyncImageAlive( std::make_shared<std::atomic<bool>>( true ) ) {
	mFlags |= UI_AUTO_SIZE;

	applyDefaultTheme();
}

UIImage::UIImage() : UIImage( "image" ) {}

UIImage::~UIImage() {
	if ( mAsyncImageAlive )
		mAsyncImageAlive->store( false, std::memory_order_release );
	clearDrawable();
}

Uint32 UIImage::getType() const {
	return UI_TYPE_IMAGE;
}

bool UIImage::isType( const Uint32& type ) const {
	return UIImage::getType() == type ? true : UIWidget::isType( type );
}

UIImage* UIImage::setDrawable( DrawablePtr drawable ) {
	if ( drawable == mDrawable )
		return this;

	Sizef oldSize( mSize );

	clearDrawable();

	mDrawable = std::move( drawable );
	sendCommonEvent( Event::OnResourceChange );

	if ( mDrawable ) {
		if ( mDrawable->getDrawableType() == Drawable::SPRITE ) {
			if ( !isSubscribedForScheduledUpdate() )
				subscribeScheduledUpdate();

			mSpriteChangeCb =
				static_cast<Sprite*>( mDrawable.get() )
					->pushEventsCallback( [this]( auto, auto, auto ) { invalidateDraw(); } );
		} else {
			if ( mDrawable->isDrawableResource() ) {
				mResourceChangeConnection =
					static_cast<DrawableResource*>( mDrawable.get() )
						->connectResourceChange(
							[this]( DrawableResource& ) { onDrawableResourceChange(); } );
			}

			if ( isSubscribedForScheduledUpdate() )
				unsubscribeScheduledUpdate();
		}
	}

	onAutoSize();

	if ( mSize != oldSize )
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );

	autoAlign();

	invalidateDraw();

	return this;
}

UIImage* UIImage::setDrawable( TexturePtr texture ) {
	return setDrawable( texture ? TextureDrawable::New( std::move( texture ) ) : DrawablePtr{} );
}

void UIImage::onAutoSize() {
	if ( nullptr == mDrawable )
		return;

	Sizef drawableSize = mFlags & UI_HTML_ELEMENT
							 ? mDrawable->getPixelsSize() * PixelDensity::getPixelDensity()
							 : mDrawable->getPixelsSize();
	if ( drawableSize.getWidth() <= 0 || drawableSize.getHeight() <= 0 )
		return;

	Sizef size( getPixelsSize() );
	if ( mFlags & UI_HTML_ELEMENT ) {
		size.x = eemax( size.x, getPropertyLength( PropertyId::LayoutWidth ) );
		size.y = eemax( size.y, getPropertyLength( PropertyId::LayoutHeight ) );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) && Sizef::Zero == getPixelsSize() )
		size = drawableSize.asInt().asFloat();

	if ( mWidthPolicy == SizePolicy::WrapContent && mHeightPolicy == SizePolicy::WrapContent ) {
		size.x = (int)drawableSize.getWidth() + mPaddingPx.Left + mPaddingPx.Right;
		size.y = (int)drawableSize.getHeight() + mPaddingPx.Top + mPaddingPx.Bottom;

		if ( !mMaxWidthEq.empty() ) {
			Float maxWidth =
				lengthFromValue( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );

			if ( size.x > maxWidth ) {
				Float scale =
					( maxWidth - mPaddingPx.Left - mPaddingPx.Right ) / drawableSize.getWidth();
				size.x = maxWidth;
				size.y =
					(int)( drawableSize.getHeight() * scale ) + mPaddingPx.Top + mPaddingPx.Bottom;
			}
		}

		if ( !mMaxHeightEq.empty() ) {
			Float maxHeight =
				lengthFromValue( mMaxHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );

			if ( size.y > maxHeight ) {
				Float scale =
					( maxHeight - mPaddingPx.Top - mPaddingPx.Bottom ) / drawableSize.getHeight();
				size.y = maxHeight;
				size.x =
					(int)( drawableSize.getWidth() * scale ) + mPaddingPx.Left + mPaddingPx.Right;
			}
		}
	} else if ( mWidthPolicy == SizePolicy::WrapContent ) {
		Float contentHeight = std::max( 0.f, size.y - mPaddingPx.Top - mPaddingPx.Bottom );
		size.x = (int)( contentHeight * ( drawableSize.getWidth() / drawableSize.getHeight() ) ) +
				 mPaddingPx.Left + mPaddingPx.Right;
		if ( !mMaxWidthEq.empty() ) {
			size.x = std::min(
				size.x,
				lengthFromValue( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth ) );
		}
	} else if ( mHeightPolicy == SizePolicy::WrapContent ) {
		Float contentWidth = std::max( 0.f, size.x - mPaddingPx.Left - mPaddingPx.Right );

		if ( !mMaxWidthEq.empty() ) {
			Float maxWidth =
				lengthFromValue( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
			Float maxContentWidth = std::max( 0.f, maxWidth - mPaddingPx.Left - mPaddingPx.Right );
			if ( contentWidth > maxContentWidth ) {
				contentWidth = maxContentWidth;
				size.x = maxWidth;
			}
		}

		size.y = (int)( contentWidth * ( drawableSize.getHeight() / drawableSize.getWidth() ) ) +
				 mPaddingPx.Top + mPaddingPx.Bottom;
		if ( !mMaxHeightEq.empty() ) {
			size.y = std::min(
				size.y, lengthFromValue( mMaxHeightEq,
										 CSS::PropertyRelativeTarget::ContainingBlockHeight ) );
		}
	}

	if ( mSize != size )
		setInternalPixelsSize( size );
}

void UIImage::calcDestSize() {
	if ( mScaleType == UIScaleType::Expand ) {
		mDestSize = Sizef( mSize.x - mPaddingPx.Left - mPaddingPx.Right,
						   mSize.y - mPaddingPx.Top - mPaddingPx.Bottom );
	} else if ( mScaleType == UIScaleType::FitInside ) {
		if ( NULL == mDrawable )
			return;

		Sizef pxSize( mDrawable->getPixelsSize() );
		Float Scale1 = ( mSize.x - mPaddingPx.Left - mPaddingPx.Right ) / pxSize.x;
		Float Scale2 = ( mSize.y - mPaddingPx.Top - mPaddingPx.Bottom ) / pxSize.y;

		if ( Scale1 < 1 || Scale2 < 1 ) {
			if ( Scale2 < Scale1 )
				Scale1 = Scale2;

			mDestSize = Sizef( pxSize.x * Scale1, pxSize.y * Scale1 );
		} else {
			mDestSize = pxSize;
		}
	} else if ( mDrawable ) {
		mDestSize = mDrawable->getPixelsSize();
	}

	mDestSize = mDestSize.floor();

	autoAlign();
}

void UIImage::draw() {
	UINode::draw();

	if ( mVisible && NULL != mDrawable && 0.f != mAlpha ) {
		calcDestSize();

		mDrawable->setColor( mColor );
		mDrawable->draw( Vector2f( std::trunc( mScreenPos.x ) + std::trunc( mAlignOffset.x ),
								   std::trunc( mScreenPos.y ) + std::trunc( mAlignOffset.y ) ),
						 mDestSize );
		mDrawable->clearColor();
	}
}

void UIImage::setAlpha( const Float& alpha ) {
	UINode::setAlpha( alpha );
	mColor.a = (Uint8)alpha;
}

const DrawablePtr& UIImage::getDrawable() const {
	return mDrawable;
}

const Color& UIImage::getColor() const {
	return mColor;
}

UIImage* UIImage::setColor( const Color& col ) {
	if ( mColor != col ) {
		mColor = col;
		setAlpha( col.a );
		invalidateDraw();
	}
	return this;
}

void UIImage::autoAlign() {
	if ( NULL == mDrawable )
		return;

	if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = ( mSize.getWidth() - mDestSize.x ) / 2;
	} else if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x = mSize.getWidth() - mDestSize.x - mPaddingPx.Right;
	} else {
		mAlignOffset.x = mPaddingPx.Left;
	}

	if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = ( mSize.getHeight() - mDestSize.y ) / 2;
	} else if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - mDestSize.y - mPaddingPx.Bottom;
	} else {
		mAlignOffset.y = mPaddingPx.Top;
	}
}

void UIImage::clearDrawable() {
	if ( mDrawable && mDrawable->getDrawableType() == Drawable::SPRITE ) {
		static_cast<Sprite*>( mDrawable.get() )->popEventsCallback( mSpriteChangeCb );
		mSpriteChangeCb = 0;
	}

	mResourceChangeConnection.disconnect();
	mDrawable.reset();
}

void UIImage::onDrawableResourceChange() {
	runOnMainThread( [this] {
		auto s = mSize;
		onAutoSize();
		calcDestSize();
		if ( mSize != s )
			notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
		invalidateDraw();
	} );
}

bool UIImage::loadFileDrawable( const Network::URI& uri ) {
	UISceneNode* scene = getUISceneNode();
	if ( !scene || !scene->getThreadPool() ||
		 !Window::Engine::instance()->isSharedGLContextEnabled() )
		return false;

	Uint64 loadId = ++mRemoteImageLoadId;
	std::string filePath = uri.getFSPath();
	std::string cacheName = getTextureCacheName( uri );
	ResourceScopePtr resourceScope = scene->getResourceScope();
	if ( TexturePtr texture = resourceScope->findTexture( cacheName ) ) {
		setDrawable( std::move( texture ) );
		return true;
	}

	auto resourceState = scene->getAsyncResourceLoadState();
	Uint64 resourceGeneration =
		resourceState ? resourceState->generation.load( std::memory_order_acquire ) : 0;
	auto alive = mAsyncImageAlive;

	scene->getThreadPool()->run( [resourceState, resourceGeneration, resourceScope, alive, loadId,
								  filePath = std::move( filePath ),
								  cacheName = std::move( cacheName ), this] {
		if ( !UISceneNode::isAsyncResourceLoadCurrent( resourceState, resourceGeneration ) ||
			 !alive || !alive->load( std::memory_order_acquire ) )
			return;

		TexturePtr texture = loadFileTextureCached( resourceScope, filePath, cacheName );
		if ( texture == nullptr )
			return;

		UISceneNode::runAsyncResourceOnMainThread(
			resourceState, resourceGeneration, [alive, loadId, texture, this]( UISceneNode* ) {
				if ( !alive || !alive->load( std::memory_order_acquire ) ||
					 loadId != mRemoteImageLoadId )
					return;

				setDrawable( std::move( texture ) );
			} );
	} );

	return true;
}

void UIImage::loadRemoteDrawable( const Network::URI& uri ) {
	UISceneNode* scene = getUISceneNode();
	if ( !scene )
		return;

	std::string url = uri.toString();
	ResourceScopePtr resourceScope = scene->getResourceScope();
	if ( TexturePtr texture = resourceScope->findTexture( url ) ) {
		TextureDrawable* textureDrawable =
			mDrawable && mDrawable->getDrawableType() == Drawable::TEXTUREDRAWABLE
				? static_cast<TextureDrawable*>( mDrawable.get() )
				: nullptr;
		if ( !textureDrawable || textureDrawable->getTexture() != texture ) {
			++mRemoteImageLoadId;
			setDrawable( std::move( texture ) );
		}
		return;
	}

	auto resourceState = scene->getAsyncResourceLoadState();
	Uint64 resourceGeneration =
		resourceState ? resourceState->generation.load( std::memory_order_acquire ) : 0;
	Uint64 loadId = ++mRemoteImageLoadId;
	auto alive = mAsyncImageAlive;
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
							 loadId != mRemoteImageLoadId )
							return;

						Image image( reinterpret_cast<const Uint8*>( imageData.data() ),
									 imageData.size() );
						if ( image.getPixels() != nullptr )
							texture->replace( &image );
					} );
			} else {
				Log::debug( "UIImage::loadRemoteDrawable: could not download image: %s. Error: "
							"%d\n%s",
							url, response.getStatus(), response.getBody() );
			}
		},
		uri, Seconds( 5 ), {}, headers, "", true, Http::getEnvProxyURI() );
}

void UIImage::onSizeChange() {
	onAutoSize();
	calcDestSize();
	UIWidget::onSizeChange();
}

void UIImage::onSizePolicyChange() {
	onAutoSize();
	UIWidget::onSizePolicyChange();
}

void UIImage::onAlignChange() {
	UIWidget::onAlignChange();
	onAutoSize();
	calcDestSize();
}

const Vector2f& UIImage::getAlignOffset() const {
	return mAlignOffset;
}

Float UIImage::getMinIntrinsicWidth() const {
	if ( mWidthPolicy == SizePolicy::Fixed )
		return getPropertyWidth();
	if ( mDrawable )
		return mDrawable->getMinIntrinsicWidth() + mPaddingPx.Left + mPaddingPx.Right;
	return mPaddingPx.Left + mPaddingPx.Right;
}

Float UIImage::getMaxIntrinsicWidth() const {
	if ( mWidthPolicy == SizePolicy::Fixed )
		return getPropertyWidth();
	if ( mDrawable )
		return mDrawable->getMaxIntrinsicWidth() + mPaddingPx.Left + mPaddingPx.Right;
	return mPaddingPx.Left + mPaddingPx.Right;
}

std::string UIImage::getPropertyString( const PropertyDefinition* propertyDef,
										const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Src:
			// TODO: Implement src.
			return "";
		case PropertyId::Icon:
			// TODO: Implement icon.
			return "";
		case PropertyId::ScaleType:
			return getScaleType() == UIScaleType::FitInside
					   ? "fit-inside"
					   : ( getScaleType() == UIScaleType::Expand ? "expand" : "none" );
		case PropertyId::Tint:
			return getColor().toHexString();
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

void UIImage::scheduledUpdate( const Time& time ) {
	if ( mDrawable && mDrawable->getDrawableType() == Drawable::SPRITE )
		static_cast<Sprite*>( mDrawable.get() )->update( time );
}

std::vector<PropertyId> UIImage::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::ScaleType, PropertyId::Tint };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIImage::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::TextAlign: {
			std::string align = String::toLower( attribute.value() );
			if ( align == "center" )
				setHorizontalAlign( UI_HALIGN_CENTER );
			else if ( align == "left" )
				setHorizontalAlign( UI_HALIGN_LEFT );
			else if ( align == "right" )
				setHorizontalAlign( UI_HALIGN_RIGHT );
			break;
		}
		case PropertyId::Src: {
			if ( attribute.getValue().empty() )
				return true;

			std::string path( attribute.getValue() );
			URI uri( path );
			UISceneNode* scene = getUISceneNode();

			if ( scene && uri.getScheme().empty() && !scene->getURI().empty() ) {
				uri = scene->solveRelativePath( uri );
				path = uri.toString();
			}

			if ( uri.getScheme() == "http" || uri.getScheme() == "https" ) {
				loadRemoteDrawable( uri );
				break;
			}

			if ( mDeferLoad && uri.getScheme() == "file" && loadFileDrawable( uri ) )
				break;

			DrawablePtr createdDrawable =
				StyleSheetSpecification::instance()->getDrawableImageParser().createDrawable(
					path, mSize, this );
			if ( createdDrawable ) {
				setDrawable( std::move( createdDrawable ) );
			} else {
				setDrawable( DrawableSearcher::searchByName(
					path, false, scene ? scene->getReferer() : URI(),
					scene ? scene->getResourceScope().get() : nullptr ) );
			}
			break;
		}
		case PropertyId::Icon: {
			std::string val = attribute.asString();
			UIIcon* iconF = getUISceneNode()->findIcon( val );
			if ( iconF ) {
				setDrawable(
					iconF->createDrawable( mSize.getHeight() - mPaddingPx.Top - mPadding.Bottom ) );
			} else if ( DrawablePtr icon = StyleSheetSpecification::instance()
											   ->getDrawableImageParser()
											   .createDrawable( val, getPixelsSize(), this ) ) {
				setDrawable( std::move( icon ) );
			}
			break;
		}
		case PropertyId::ScaleType: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );
			if ( "expand" == val ) {
				setScaleType( UIScaleType::Expand );
			} else if ( "fit-inside" == val || "fit_inside" == val || "fitinside" == val ) {
				setScaleType( UIScaleType::FitInside );
			} else if ( "none" == val ) {
				setScaleType( UIScaleType::None );
			}
			break;
		}
		case PropertyId::Tint:
			setColor( attribute.asColor() );
			break;
		case PropertyId::Width:
		case PropertyId::Height:
			unsetFlags( UI_AUTO_SIZE );
			return UIWidget::applyProperty( attribute );
		case PropertyId::Defer:
			mDeferLoad = attribute.getValue().empty() ? true : attribute.asBool();
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

const UIScaleType& UIImage::getScaleType() const {
	return mScaleType;
}

UIImage* UIImage::setScaleType( const UIScaleType& scaleType ) {
	mScaleType = scaleType;
	calcDestSize();
	invalidateDraw();
	return this;
}

}} // namespace EE::UI
