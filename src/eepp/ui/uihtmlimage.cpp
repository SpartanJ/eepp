#include "vectorraster.hpp"
#include <eepp/ui/accessibility/accessibilitywidgetresolver.hpp>
#include <eepp/ui/uihtmlimage.hpp>
#define PUGIXML_HEADER_ONLY
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/texturedrawable.hpp>
#include <eepp/network/http.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/drawableimageparser.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIHTMLImage* UIHTMLImage::New() {
	return eeNew( UIHTMLImage, () );
}

UIHTMLImage::UIHTMLImage() : UIHTMLWidget( "img" ) {
	mFlags |= UI_HTML_ELEMENT;
	mWidthPolicy = mHeightPolicy = SizePolicy::WrapContent;
	setDisplay( CSSDisplay::Inline );
}

UIHTMLImage::~UIHTMLImage() {
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
	const Uint32 textHints = getDefaultTextHints();
	Float width = Text::getTextWidth( mAlt, style, 4, textHints );
	Float available = mSize.x - mPaddingPx.Left - mPaddingPx.Right;
	Float x = mScreenPos.x + mPaddingPx.Left + eemax( 0.f, ( available - width ) * 0.5f );
	Float y = mScreenPos.y + mPaddingPx.Top +
			  ( mSize.y - mPaddingPx.Top - mPaddingPx.Bottom -
				PixelDensity::getPixelDensity() * style.CharacterSize ) *
				  0.5f;
	Text::draw( String( mAlt ), { x, y }, style, 4, textHints );
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
	// The raster helper belongs to one source; the src handler sets it again.
	mVectorRaster.reset();
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
		}
	}
	if ( ( !mDrawable || mDrawable->getDrawableType() != Drawable::SPRITE ) &&
		 isSubscribedForScheduledUpdate() )
		unsubscribeScheduledUpdate();
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
			if ( property.getValue().empty() ) {
				setDrawable( DrawablePtr{} );
				break;
			}
			std::string path( property.getValue() );
			URI uri( path );
			UISceneNode* scene = getUISceneNode();
			if ( scene && uri.getScheme().empty() && !scene->getURI().empty() ) {
				uri = scene->solveRelativePath( uri );
				path = uri.toString();
			}
			if ( uri.getScheme() == "http" || uri.getScheme() == "https" ) {
				loadRemoteDrawable( uri );
			} else if ( DrawablePtr deferred =
							mDeferLoad && scene && uri.getScheme() == "file"
								? scene->getDrawableResolver().resolveAsync( path )
								: DrawablePtr{} ) {
				setDrawable( std::move( deferred ) );
			} else if ( DrawablePtr drawable = StyleSheetSpecification::instance()
												   ->getDrawableImageParser()
												   .createDrawable( path, mSize, this ) ) {
				setDrawable( std::move( drawable ) );
			} else if ( scene ) {
				setDrawable( scene->getDrawableResolver().resolve( path ) );
			} else {
				DrawableResolver resolver( defaultResourceScope() );
				setDrawable( resolver.resolve( path ) );
			}
			mVectorRaster = VectorRaster::create( this, std::move( path ), [this] {
				return VectorRaster::textureDrawableOf( mDrawable );
			} );
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
	Float width = 0.f;
	if ( getDrawable() && getDrawable()->isVectorRaster() ) {
		width = layoutDrawableSize().x;
	} else if ( getDrawable() ) {
		width = getDrawable()->getMinIntrinsicWidth() * PixelDensity::getPixelDensity() /
				getDrawable()->getRasterScale();
	}
	return width + mPaddingPx.Left + mPaddingPx.Right;
}

Float UIHTMLImage::getMaxIntrinsicWidth() const {
	if ( mWidthPolicy == SizePolicy::Fixed )
		return getPropertyWidth();
	Float width = 0.f;
	if ( getDrawable() && getDrawable()->isVectorRaster() ) {
		width = layoutDrawableSize().x;
	} else if ( getDrawable() ) {
		width = getDrawable()->getMaxIntrinsicWidth() * PixelDensity::getPixelDensity() /
				getDrawable()->getRasterScale();
	}
	return width + mPaddingPx.Left + mPaddingPx.Right;
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

Sizef UIHTMLImage::layoutDrawableSize() const {
	if ( !mDrawable )
		return Sizef::Zero;
	// An SVG raster made for the drawn size reports its SVG's natural size, so rerasterizing
	// doesn't change layout.
	if ( mDrawable->isVectorRaster() )
		return mDrawable->getNaturalPixelsSize();
	return mDrawable->getPixelsSize() *
		   ( PixelDensity::getPixelDensity() / mDrawable->getRasterScale() );
}

void UIHTMLImage::autoSizeImage() {
	Sizef drawableSize = layoutDrawableSize();
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
		Sizef pixels( mDrawable->getNaturalPixelsSize() );
		mDestSize = pixels;
		if ( pixels.x > 0 && pixels.y > 0 ) {
			Float scale = eemin( ( mSize.x - mPaddingPx.Left - mPaddingPx.Right ) / pixels.x,
								 ( mSize.y - mPaddingPx.Top - mPaddingPx.Bottom ) / pixels.y );
			if ( scale < 1.f )
				mDestSize *= scale;
		}
	} else if ( mDrawable ) {
		mDestSize = mDrawable->getNaturalPixelsSize();
	}
	mDestSize = mDestSize.floor();
	mAlignOffset = { mPaddingPx.Left, mPaddingPx.Top };

	if ( mVectorRaster )
		mVectorRaster->update( mDestSize );
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
		// A deferred load that failed leaves an empty texture: drop it to show the alt text, as a
		// failed synchronous load does.
		if ( mDrawable && mDrawable->getDrawableType() == Drawable::TEXTUREDRAWABLE &&
			 !static_cast<TextureDrawable*>( mDrawable.get() )->getTexture() ) {
			setDrawable( DrawablePtr{} );
			return;
		}
		Sizef oldSize( mSize );
		autoSizeImage();
		calcDestSize();
		if ( mSize != oldSize )
			notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
		invalidateIntrinsicSize();
		invalidateDraw();
	} );
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
		if ( !current || current->getTexture() != texture )
			setDrawable( std::move( texture ) );
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
		if ( !current || current->getTexture() != texture )
			setDrawable( std::move( texture ) );
	}
}

const std::string& UIHTMLImage::getAlt() const {
	return mAlt;
}

UIHTMLImage* UIHTMLImage::setAlt( const std::string& alt ) {
	if ( mAlt != alt ) {
		const auto previousRole =
			Accessibility::isActive() ? getAccessibilityRole() : AccessibilityRole::None;
		mAlt = alt;
		if ( Accessibility::isActive() )
			AccessibilityWidgetResolver::onHTMLTextChanged( this, previousRole );
		invalidateDraw();
	}
	return this;
}

bool UIHTMLImage::isInline() const {
	return getDisplay() == CSSDisplay::Inline && getCSSFloat() == CSSFloat::None && !isOutOfFlow();
}

}} // namespace EE::UI
