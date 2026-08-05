#include <eepp/ui/uihtmlimage.hpp>
#define PUGIXML_HEADER_ONLY
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/texturedrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/network/http.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/drawableimageparser.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <pugixml/pugixml.hpp>

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
		 filePath[2] == ':' )
		filePath = filePath.substr( 1 );
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

UIHTMLImage* UIHTMLImage::New() {
	return eeNew( UIHTMLImage, () );
}

UIHTMLImage::UIHTMLImage() : UIHTMLWidget( "img" ) {
	mFlags |= UI_HTML_ELEMENT;
	mWidthPolicy = mHeightPolicy = SizePolicy::WrapContent;
	setDisplay( CSSDisplay::Inline );
}

UIHTMLImage::~UIHTMLImage() {
	if ( mAsyncImageAlive )
		mAsyncImageAlive->store( false, std::memory_order_release );
	clearDrawable();
}

Uint32 UIHTMLImage::getType() const {
	return UI_TYPE_HTML_IMAGE;
}

bool UIHTMLImage::isType( const Uint32& type ) const {
	return type == getType() || UIHTMLWidget::isType( type );
}

void UIHTMLImage::loadFromXmlNode( const pugi::xml_node& node ) {
	for ( auto& attr : node.attributes() )
		if ( String::iequals( attr.name(), "alt" ) ) {
			mAlt = attr.value();
			break;
		}
	UIHTMLWidget::loadFromXmlNode( node );
}

void UIHTMLImage::draw() {
	if ( mVisible && getDrawable() && mAlpha != 0.f ) {
		UIHTMLWidget::draw();
		calcDestSize();
		mDrawable->setColor( mColor );
		mDrawable->draw( { std::trunc( mScreenPos.x ) + std::trunc( mAlignOffset.x ),
						   std::trunc( mScreenPos.y ) + std::trunc( mAlignOffset.y ) },
						 mDestSize );
		mDrawable->clearColor();
		return;
	}
	if ( !mVisible || mAlpha == 0.f || mAlt.empty() )
		return;
	UINode::draw();
	auto* theme = getUISceneNode()->getUIThemeManager();
	FontStyleConfig style;
	style.Font = theme->getDefaultFont();
	style.CharacterSize = theme->getDefaultFontSize();
	Color color = Color::White;
	for ( Node* parent = mParentNode; parent; parent = parent->getParent() )
		if ( parent->isType( UI_TYPE_RICHTEXT ) ) {
			color = parent->asType<UIRichText>()->getFontColor();
			break;
		}
	style.FontColor = { color.r, color.g, color.b, static_cast<Uint8>( mAlpha ) };
	Float width = Text::getTextWidth( mAlt, style );
	Float available = mSize.x - mPaddingPx.Left - mPaddingPx.Right;
	Float x = mScreenPos.x + mPaddingPx.Left + eemax( 0.f, ( available - width ) * 0.5f );
	Float y = mScreenPos.y + mPaddingPx.Top +
			  ( mSize.y - mPaddingPx.Top - mPaddingPx.Bottom -
				PixelDensity::getPixelDensity() * style.CharacterSize ) *
				  0.5f;
	Text::draw( String( mAlt ), { x, y }, style );
}

void UIHTMLImage::setAlpha( const Float& alpha ) {
	UINode::setAlpha( alpha );
	mColor.a = static_cast<Uint8>( alpha );
}

const DrawablePtr& UIHTMLImage::getDrawable() const {
	return mDrawable;
}

UIHTMLImage* UIHTMLImage::setDrawable( DrawablePtr drawable ) {
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
			if ( mDrawable->isDrawableResource() )
				mResourceChangeConnection =
					static_cast<DrawableResource*>( mDrawable.get() )
						->connectResourceChange(
							[this]( DrawableResource& ) { onDrawableResourceChange(); } );
			if ( isSubscribedForScheduledUpdate() )
				unsubscribeScheduledUpdate();
		}
	}
	autoSizeImage();
	if ( mSize != oldSize )
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	calcDestSize();
	invalidateIntrinsicSize();
	invalidateDraw();
	return this;
}

UIHTMLImage* UIHTMLImage::setDrawable( TexturePtr texture ) {
	return setDrawable( texture ? TextureDrawable::New( std::move( texture ) ) : DrawablePtr{} );
}

const Color& UIHTMLImage::getColor() const {
	return mColor;
}

UIHTMLImage* UIHTMLImage::setColor( const Color& color ) {
	if ( mColor != color ) {
		mColor = color;
		UINode::setAlpha( color.a );
		invalidateDraw();
	}
	return this;
}

const Vector2f& UIHTMLImage::getAlignOffset() const {
	return mAlignOffset;
}

bool UIHTMLImage::applyProperty( const StyleSheetProperty& property ) {
	if ( !checkPropertyDefinition( property ) )
		return false;
	const PropertyId propertyId = property.getPropertyDefinition()->getPropertyId();
	switch ( propertyId ) {
		case PropertyId::Src: {
			if ( property.getValue().empty() )
				return true;
			std::string path( property.getValue() );
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
			DrawablePtr drawable =
				StyleSheetSpecification::instance()->getDrawableImageParser().createDrawable(
					path, mSize, this );
			if ( drawable )
				setDrawable( std::move( drawable ) );
			else if ( scene )
				setDrawable( scene->getDrawableResolver().resolve( path ) );
			else {
				DrawableResolver resolver( defaultResourceScope() );
				setDrawable( resolver.resolve( path ) );
			}
			break;
		}
		case PropertyId::ScaleType: {
			const auto& value = property.getValue();
			if ( String::iequals( value, "expand" ) )
				setScaleType( UIScaleType::Expand );
			else if ( String::iequals( value, "fit-inside" ) ||
					  String::iequals( value, "fit_inside" ) ||
					  String::iequals( value, "fitinside" ) )
				setScaleType( UIScaleType::FitInside );
			else if ( String::iequals( value, "none" ) )
				setScaleType( UIScaleType::None );
			break;
		}
		case PropertyId::Tint:
			setColor( property.asColor() );
			break;
		case PropertyId::Defer:
			mDeferLoad = property.getValue().empty() || property.asBool();
			break;
		default: {
			if ( isInAttributesTransaction() ) {
				switch ( propertyId ) {
					case PropertyId::Width:
					case PropertyId::Height:
					case PropertyId::MinWidth:
					case PropertyId::MinHeight:
					case PropertyId::MaxWidth:
					case PropertyId::MaxHeight:
					case PropertyId::PaddingLeft:
					case PropertyId::PaddingRight:
					case PropertyId::PaddingTop:
					case PropertyId::PaddingBottom:
					case PropertyId::BorderLeftWidth:
					case PropertyId::BorderRightWidth:
					case PropertyId::BorderTopWidth:
					case PropertyId::BorderBottomWidth:
					case PropertyId::BoxSizing:
						mNeedsStyleSizeReconciliation = true;
						break;
					default:
						break;
				}
			}
			return UIHTMLWidget::applyProperty( property );
		}
	}
	return true;
}

Float UIHTMLImage::getMinIntrinsicWidth() const {
	if ( mWidthPolicy == SizePolicy::Fixed )
		return getPropertyWidth();
	return ( getDrawable() ? getDrawable()->getMinIntrinsicWidth() * PixelDensity::getPixelDensity()
						   : 0.f ) +
		   mPaddingPx.Left + mPaddingPx.Right;
}

Float UIHTMLImage::getMaxIntrinsicWidth() const {
	if ( mWidthPolicy == SizePolicy::Fixed )
		return getPropertyWidth();
	return ( getDrawable() ? getDrawable()->getMaxIntrinsicWidth() * PixelDensity::getPixelDensity()
						   : 0.f ) +
		   mPaddingPx.Left + mPaddingPx.Right;
}

std::string UIHTMLImage::getPropertyString( const PropertyDefinition* property,
											const Uint32& index ) const {
	if ( !property )
		return "";
	if ( property->getPropertyId() == PropertyId::ScaleType )
		return getScaleType() == UIScaleType::FitInside
				   ? "fit-inside"
				   : ( getScaleType() == UIScaleType::Expand ? "expand" : "none" );
	if ( property->getPropertyId() == PropertyId::Tint )
		return getColor().toHexString();
	return UIHTMLWidget::getPropertyString( property, index );
}

void UIHTMLImage::scheduledUpdate( const Time& time ) {
	if ( mDrawable && mDrawable->getDrawableType() == Drawable::SPRITE )
		static_cast<Sprite*>( mDrawable.get() )->update( time );
}

void UIHTMLImage::updateLayout() {
	reconcileStyleSize();
	autoSizeImage();
	UIHTMLWidget::updateLayout();
}

std::vector<PropertyId> UIHTMLImage::getPropertiesImplemented() const {
	auto properties = UIHTMLWidget::getPropertiesImplemented();
	properties.insert( properties.end(), { PropertyId::ScaleType, PropertyId::Tint } );
	return properties;
}

const UIScaleType& UIHTMLImage::getScaleType() const {
	return mScaleType;
}

UIHTMLImage* UIHTMLImage::setScaleType( const UIScaleType& type ) {
	if ( mScaleType != type ) {
		mScaleType = type;
		calcDestSize();
		invalidateDraw();
	}
	return this;
}

void UIHTMLImage::autoSizeImage() {
	Sizef drawableSize =
		mDrawable ? mDrawable->getPixelsSize() * PixelDensity::getPixelDensity() : Sizef::Zero;
	if ( drawableSize.x <= 0.f || drawableSize.y <= 0.f )
		return;
	Sizef size = getPixelsSize();
	if ( mWidthPolicy == SizePolicy::WrapContent && mHeightPolicy == SizePolicy::WrapContent ) {
		size = { drawableSize.x + mPaddingPx.Left + mPaddingPx.Right,
				 drawableSize.y + mPaddingPx.Top + mPaddingPx.Bottom };
		if ( !mMaxWidthEq.empty() ) {
			Float maxWidth =
				lengthFromValue( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
			if ( maxWidth > 0.f && size.x > maxWidth ) {
				Float scale = ( maxWidth - mPaddingPx.Left - mPaddingPx.Right ) / drawableSize.x;
				size = { maxWidth, drawableSize.y * scale + mPaddingPx.Top + mPaddingPx.Bottom };
			}
		}
		if ( !mMaxHeightEq.empty() ) {
			Float maxHeight =
				lengthFromValue( mMaxHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
			if ( maxHeight > 0.f && size.y > maxHeight ) {
				Float scale = ( maxHeight - mPaddingPx.Top - mPaddingPx.Bottom ) / drawableSize.y;
				size = { drawableSize.x * scale + mPaddingPx.Left + mPaddingPx.Right, maxHeight };
			}
		}
	} else if ( mWidthPolicy == SizePolicy::WrapContent ) {
		Float contentHeight = eemax( 0.f, size.y - mPaddingPx.Top - mPaddingPx.Bottom );
		size.x =
			contentHeight * drawableSize.x / drawableSize.y + mPaddingPx.Left + mPaddingPx.Right;
		if ( !mMaxWidthEq.empty() ) {
			Float maxWidth =
				lengthFromValue( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
			if ( maxWidth > 0.f )
				size.x = eemin( size.x, maxWidth );
		}
	} else if ( mHeightPolicy == SizePolicy::WrapContent ) {
		Float contentWidth = eemax( 0.f, size.x - mPaddingPx.Left - mPaddingPx.Right );
		if ( !mMaxWidthEq.empty() ) {
			Float maxWidth =
				lengthFromValue( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
			Float maxContent = eemax( 0.f, maxWidth - mPaddingPx.Left - mPaddingPx.Right );
			if ( maxWidth > 0.f && contentWidth > maxContent ) {
				contentWidth = maxContent;
				size.x = maxWidth;
			}
		}
		size.y =
			contentWidth * drawableSize.y / drawableSize.x + mPaddingPx.Top + mPaddingPx.Bottom;
		if ( !mMaxHeightEq.empty() ) {
			Float maxHeight =
				lengthFromValue( mMaxHeightEq, CSS::PropertyRelativeTarget::ContainingBlockHeight );
			if ( maxHeight > 0.f )
				size.y = eemin( size.y, maxHeight );
		}
	}
	if ( mSize != size )
		setInternalPixelsSize( size.floor() );
}

bool UIHTMLImage::hasUnresolvedPercentageSize() const {
	const auto* style = getUIStyle();
	const auto* width = style ? style->getProperty( PropertyId::Width ) : nullptr;
	const auto* height = style ? style->getProperty( PropertyId::Height ) : nullptr;
	const bool needsContainingWidth =
		( width && StyleSheetLength::isPercentage( width->value() ) ) ||
		StyleSheetLength::isPercentage( mMinWidthEq ) ||
		StyleSheetLength::isPercentage( mMaxWidthEq );
	const bool needsContainingHeight =
		( height && StyleSheetLength::isPercentage( height->value() ) ) ||
		StyleSheetLength::isPercentage( mMinHeightEq ) ||
		StyleSheetLength::isPercentage( mMaxHeightEq );
	return ( needsContainingWidth && getContainingBlockContentWidth() <= 0.f ) ||
		   ( needsContainingHeight && getContainingBlockContentHeight() <= 0.f );
}

void UIHTMLImage::reconcileStyleSize() {
	if ( !mNeedsStyleSizeReconciliation || hasUnresolvedPercentageSize() )
		return;
	mNeedsStyleSizeReconciliation = false;
	updateCSSContentBoxFixedSize();
	autoSizeImage();
}

void UIHTMLImage::onSizeChange() {
	autoSizeImage();
	calcDestSize();
	UIHTMLWidget::onSizeChange();
}

void UIHTMLImage::onSizePolicyChange() {
	autoSizeImage();
	UIHTMLWidget::onSizePolicyChange();
}

void UIHTMLImage::onAlignChange() {
	UIHTMLWidget::onAlignChange();
	calcDestSize();
}

void UIHTMLImage::onParentSizeChange( const Vector2f& change ) {
	UIHTMLWidget::onParentSizeChange( change );
	reconcileStyleSize();
	autoSizeImage();
}

void UIHTMLImage::onDisplayChange() {
	UIHTMLWidget::onDisplayChange();
	// CSS 2.2 §10.3.4: a block-level replaced element with width:auto uses its intrinsic width;
	// unlike an ordinary block box, it does not fill the containing block. max-width is applied
	// after decoding by autoSizeImage().
	const auto* width = getUIStyle() ? getUIStyle()->getProperty( PropertyId::Width ) : nullptr;
	if ( getLayoutWidthPolicy() == SizePolicy::MatchParent &&
		 ( width == nullptr || width->value() == "auto" ) )
		setLayoutWidthPolicy( SizePolicy::WrapContent );
}

void UIHTMLImage::onAttributesTransactionEnd() {
	UIHTMLWidget::onAttributesTransactionEnd();
	// Replaced sizing depends on the final width/height policies, min/max
	// constraints and intrinsic ratio, so reconcile it after the complete style
	// transaction instead of depending on property traversal order.
	reconcileStyleSize();
}

void UIHTMLImage::calcDestSize() {
	if ( mScaleType == UIScaleType::Expand ) {
		mDestSize = { mSize.x - mPaddingPx.Left - mPaddingPx.Right,
					  mSize.y - mPaddingPx.Top - mPaddingPx.Bottom };
	} else if ( mScaleType == UIScaleType::FitInside && mDrawable ) {
		Sizef pixels( mDrawable->getPixelsSize() );
		Float scale = eemin( ( mSize.x - mPaddingPx.Left - mPaddingPx.Right ) / pixels.x,
							 ( mSize.y - mPaddingPx.Top - mPaddingPx.Bottom ) / pixels.y );
		mDestSize = scale < 1.f ? pixels * scale : pixels;
	} else if ( mDrawable ) {
		mDestSize = mDrawable->getPixelsSize();
	}
	mDestSize = mDestSize.floor();
	mAlignOffset = { mPaddingPx.Left, mPaddingPx.Top };
}

void UIHTMLImage::clearDrawable() {
	if ( mDrawable && mDrawable->getDrawableType() == Drawable::SPRITE ) {
		static_cast<Sprite*>( mDrawable.get() )->popEventsCallback( mSpriteChangeCb );
		mSpriteChangeCb = 0;
	}
	mResourceChangeConnection.disconnect();
	mDrawable.reset();
}

void UIHTMLImage::onDrawableResourceChange() {
	runOnMainThread( [this] {
		Sizef oldSize( mSize );
		autoSizeImage();
		calcDestSize();
		if ( mSize != oldSize )
			notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
		invalidateIntrinsicSize();
		invalidateDraw();
	} );
}

bool UIHTMLImage::loadFileDrawable( const Network::URI& uri ) {
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
	Uint64 generation =
		resourceState ? resourceState->generation.load( std::memory_order_acquire ) : 0;
	if ( !mAsyncImageAlive )
		mAsyncImageAlive = std::make_shared<std::atomic<bool>>( true );
	auto alive = mAsyncImageAlive;
	scene->getThreadPool()->run( [resourceState, generation, resourceScope, alive, loadId,
								  filePath = std::move( filePath ),
								  cacheName = std::move( cacheName ), this] {
		if ( !UISceneNode::isAsyncResourceLoadCurrent( resourceState, generation ) || !alive ||
			 !alive->load( std::memory_order_acquire ) )
			return;
		TexturePtr texture = loadFileTextureCached( resourceScope, filePath, cacheName );
		if ( !texture )
			return;
		UISceneNode::runAsyncResourceOnMainThread(
			resourceState, generation, [alive, loadId, texture, this]( UISceneNode* ) mutable {
				if ( !alive || !alive->load( std::memory_order_acquire ) ||
					 loadId != mRemoteImageLoadId )
					return;
				setDrawable( std::move( texture ) );
			} );
	} );
	return true;
}

void UIHTMLImage::loadRemoteDrawable( const Network::URI& uri ) {
	UISceneNode* scene = getUISceneNode();
	if ( !scene )
		return;
	std::string url = uri.toString();
	if ( TexturePtr texture = scene->getResourceScope()->findTexture( url ) ) {
		TextureDrawable* current =
			mDrawable && mDrawable->getDrawableType() == Drawable::TEXTUREDRAWABLE
				? static_cast<TextureDrawable*>( mDrawable.get() )
				: nullptr;
		if ( !current || current->getTexture() != texture ) {
			++mRemoteImageLoadId;
			setDrawable( std::move( texture ) );
		}
		return;
	}
	WebResourceRequest request;
	request.uri = uri;
	request.kind = WebResourceKind::Image;
	request.proxy = Http::getEnvProxyURI();
	TexturePtr texture = scene->requestWebTexture(
		std::move( request ), [url = std::move( url )]( const WebResourceResult& result ) {
			if ( !result.success )
				Log::debug( "UIHTMLImage: could not download image: %s. Error: %d\n%s", url,
							result.status, result.error );
		} );
	if ( texture ) {
		TextureDrawable* current =
			mDrawable && mDrawable->getDrawableType() == Drawable::TEXTUREDRAWABLE
				? static_cast<TextureDrawable*>( mDrawable.get() )
				: nullptr;
		if ( !current || current->getTexture() != texture ) {
			++mRemoteImageLoadId;
			setDrawable( std::move( texture ) );
		}
	}
}

const std::string& UIHTMLImage::getAlt() const {
	return mAlt;
}

UIHTMLImage* UIHTMLImage::setAlt( const std::string& alt ) {
	if ( mAlt != alt ) {
		mAlt = alt;
		invalidateDraw();
	}
	return this;
}

bool UIHTMLImage::isInline() const {
	return getDisplay() == CSSDisplay::Inline && getCSSFloat() == CSSFloat::None && !isOutOfFlow();
}

}} // namespace EE::UI
