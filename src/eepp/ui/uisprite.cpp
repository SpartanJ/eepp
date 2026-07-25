#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/texturedrawable.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisprite.hpp>

namespace EE { namespace UI {

UISprite* UISprite::New() {
	return eeNew( UISprite, () );
}

UISprite::UISprite() :
	UIWidget( "sprite" ),
	mRender( RENDER_NORMAL ),
	mAlignOffset( 0, 0 ),
	mTextureRegionLast( NULL ) {
	subscribeScheduledUpdate();
}

UISprite::~UISprite() {}

Uint32 UISprite::getType() const {
	return UI_TYPE_SPRITE;
}

bool UISprite::isType( const Uint32& type ) const {
	return UISprite::getType() == type ? true : UIWidget::isType( type );
}

UISprite* UISprite::setSprite( Graphics::SpritePtr sprite ) {
	mSprite = std::move( sprite );
	if ( mSprite )
		mSprite->setAutoAnimate( false );

	updateSize();
	return this;
}

void UISprite::draw() {
	UIWidget::draw();

	if ( mVisible ) {
		if ( NULL != mSprite && 0.f != mAlpha ) {
			checkTextureRegionUpdate();

			mSprite->setPosition(
				Vector2f( (Float)( std::trunc( mScreenPos.x ) + (int)mAlignOffset.x ),
						  (Float)( std::trunc( mScreenPos.y ) + (int)mAlignOffset.y ) ) );

			TextureRegion* textureRegion = mSprite->getCurrentTextureRegion();

			if ( NULL != textureRegion ) {
				Sizef oDestSize = textureRegion->getDestSize();
				Sizef pxSize = textureRegion->getPixelsSize();

				textureRegion->setDestSize( Sizef( (Float)pxSize.x, (Float)pxSize.y ) );

				mSprite->draw( getBlendMode(), mRender );

				textureRegion->setDestSize( oDestSize );
			}
		}
	}
}

void UISprite::scheduledUpdate( const Time& time ) {
	if ( NULL != mSprite ) {
		TextureRegion* textureRegion = mSprite->getCurrentTextureRegion();

		mSprite->update( time );

		if ( textureRegion != mSprite->getCurrentTextureRegion() )
			invalidateDraw();
	}
}

void UISprite::checkTextureRegionUpdate() {
	if ( NULL != mSprite && NULL != mSprite->getCurrentTextureRegion() &&
		 mSprite->getCurrentTextureRegion() != mTextureRegionLast ) {
		updateSize();
		autoAlign();
		mTextureRegionLast = mSprite->getCurrentTextureRegion();
	}
}

void UISprite::setAlpha( const Float& alpha ) {
	if ( NULL != mSprite )
		mSprite->setAlpha( alpha );

	UIWidget::setAlpha( alpha );
}

const Graphics::SpritePtr& UISprite::getSprite() const {
	return mSprite;
}

Color UISprite::getColor() const {
	if ( NULL != mSprite )
		return mSprite->getColor();

	return Color::White;
}

UISprite* UISprite::setColor( const Color& color ) {
	if ( NULL != mSprite )
		mSprite->setColor( color );

	setAlpha( color.a );
	return this;
}

const RenderMode& UISprite::getRenderMode() const {
	return mRender;
}

UISprite* UISprite::setRenderMode( const RenderMode& render ) {
	mRender = render;
	invalidateDraw();
	return this;
}

void UISprite::updateSize() {
	if ( NULL != mSprite ) {
		if ( mFlags & UI_AUTO_SIZE ) {
			if ( NULL != mSprite->getCurrentTextureRegion() &&
				 mSprite->getCurrentTextureRegion()->getDpSize().asFloat() != getSize() )
				setSize( mSprite->getCurrentTextureRegion()->getDpSize().asFloat() );
		}

		if ( NULL != mSprite->getCurrentTextureRegion() ) {
			if ( mWidthPolicy == SizePolicy::WrapContent ) {
				setInternalPixelsWidth(
					mSprite->getCurrentTextureRegion()->getPixelsSize().getWidth() +
					mPaddingPx.Left + mPaddingPx.Right );
			}

			if ( mHeightPolicy == SizePolicy::WrapContent ) {
				setInternalPixelsHeight(
					mSprite->getCurrentTextureRegion()->getPixelsSize().getHeight() +
					mPaddingPx.Top + mPaddingPx.Bottom );
			}
		}
	}
}

void UISprite::autoAlign() {
	if ( NULL == mSprite || NULL == mSprite->getCurrentTextureRegion() )
		return;

	TextureRegion* tTextureRegion = mSprite->getCurrentTextureRegion();

	if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = ( mSize.getWidth() - tTextureRegion->getPixelsSize().getWidth() ) / 2;
	} else if ( Font::getHorizontalAlign( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =
			mSize.getWidth() - tTextureRegion->getPixelsSize().getWidth() - mPaddingPx.Right;
	} else {
		mAlignOffset.x = mPaddingPx.Left;
	}

	if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = ( mSize.getHeight() - tTextureRegion->getPixelsSize().getHeight() ) / 2;
	} else if ( Font::getVerticalAlign( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y =
			mSize.getHeight() - tTextureRegion->getPixelsSize().getHeight() - mPaddingPx.Bottom;
	} else {
		mAlignOffset.y = mPaddingPx.Top;
	}
}

const Vector2f& UISprite::getAlignOffset() const {
	return mAlignOffset;
}

void UISprite::onSizeChange() {
	autoAlign();
	notifyLayoutAttrChange( LayoutInvalidation::Self );
	UIWidget::onSizeChange();
}

std::string UISprite::getPropertyString( const PropertyDefinition* propertyDef,
										 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Src:
			// TODO: Implement src
			return "";
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UISprite::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Src };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UISprite::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Src: {
			std::string path( attribute.getValue() );

			FunctionString func( FunctionString::parse( path ) );
			if ( !func.getParameters().empty() && func.getName() == "url" ) {
				path = func.getParameters().at( 0 );
			}

			UISceneNode* scene = getUISceneNode();
			DrawablePtr res;
			if ( scene ) {
				res = scene->getDrawableResolver().resolve( path, true );
			} else {
				DrawableResolver resolver( defaultResourceScope() );
				res = resolver.resolve( path, true );
			}

			if ( res ) {
				switch ( res->getDrawableType() ) {
					case Drawable::SPRITE:
						setSprite( std::static_pointer_cast<Sprite>( std::move( res ) ) );
						break;
					case Drawable::TEXTUREREGION:
						setSprite(
							Sprite::New( std::static_pointer_cast<TextureRegion>( res ).get() ) );
						break;
					case Drawable::TEXTUREDRAWABLE: {
						auto sprite = Sprite::New();
						sprite->createStatic(
							std::static_pointer_cast<TextureDrawable>( res )->getTexture() );
						setSprite( std::move( sprite ) );
						break;
					}
					case Drawable::TEXTURE: {
						auto sprite = Sprite::New();
						sprite->createStatic( std::static_pointer_cast<Texture>( res ) );
						setSprite( std::move( sprite ) );
						break;
					}
					default:
						break;
				}
			}
			break;
		}
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

}} // namespace EE::UI
