#include <eepp/ui/uisprite.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>

namespace EE { namespace UI {

UISprite * UISprite::New() {
	return eeNew( UISprite, () );
}

UISprite::UISprite() :
	UIWidget( "sprite" ),
	mSprite( NULL ),
	mRender( RENDER_NORMAL ),
	mAlignOffset(0,0),
	mTextureRegionLast(NULL)
{
	subscribeScheduledUpdate();
}

UISprite::~UISprite() {
	if ( deallocSprite() )
		eeSAFE_DELETE( mSprite );
}

Uint32 UISprite::getType() const {
	return UI_TYPE_SPRITE;
}

bool UISprite::isType( const Uint32& type ) const {
	return UISprite::getType() == type ? true : UIWidget::isType( type );
}

Uint32 UISprite::deallocSprite() {
	return mNodeFlags & NODE_FLAG_FREE_USE;
}

void UISprite::setSprite( Graphics::Sprite * sprite ) {
	if ( deallocSprite() )
		eeSAFE_DELETE( mSprite );

	mSprite = sprite;
	mSprite->setAutoAnimate( false );

	updateSize();
}

void UISprite::draw() {
	UIWidget::draw();

	if ( mVisible ) {
		if ( NULL != mSprite && 0.f != mAlpha ) {
			checkTextureRegionUpdate();

			mSprite->setPosition( Vector2f( (Float)( mScreenPosi.x + (int)mAlignOffset.x ), (Float)( mScreenPosi.y + (int)mAlignOffset.y ) ) );

			TextureRegion * textureRegion = mSprite->getCurrentTextureRegion();

			if ( NULL != textureRegion ) {
				Sizef oDestSize = textureRegion->getDestSize();
				Sizei pxSize = textureRegion->getPxSize();

				textureRegion->setDestSize( Sizef( (Float)pxSize.x, (Float)pxSize.y ) );

				mSprite->draw( getBlendMode(), mRender );

				textureRegion->setDestSize( oDestSize );
			}
		}
	}
}

void UISprite::scheduledUpdate( const Time& time ) {
	if ( NULL != mSprite ) {
		TextureRegion * textureRegion = mSprite->getCurrentTextureRegion();

		mSprite->update( time );

		if ( textureRegion != mSprite->getCurrentTextureRegion() )
			invalidateDraw();
	}
}

void UISprite::checkTextureRegionUpdate() {
	if ( NULL != mSprite && NULL != mSprite->getCurrentTextureRegion() && mSprite->getCurrentTextureRegion() != mTextureRegionLast ) {
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

Graphics::Sprite * UISprite::getSprite() const {
	return mSprite;
}

Color UISprite::getColor() const {
	if ( NULL != mSprite )
		return mSprite->getColor();

	return Color::White;
}

void UISprite::setColor( const Color& color ) {
	if ( NULL != mSprite )
		mSprite->setColor( color );

	setAlpha( color.a );
}

const RenderMode& UISprite::getRenderMode() const {
	return mRender;
}

void UISprite::setRenderMode( const RenderMode& render ) {
	mRender = render;
	invalidateDraw();
}

void UISprite::updateSize() {
	if ( NULL != mSprite ) {
		if ( mFlags & UI_AUTO_SIZE ) {
			if ( NULL != mSprite->getCurrentTextureRegion() && mSprite->getCurrentTextureRegion()->getDpSize().asFloat() != getSize() )
				setSize( mSprite->getCurrentTextureRegion()->getDpSize().asFloat() );
		}

		if ( NULL != mSprite->getCurrentTextureRegion() ) {
			if ( mLayoutWidthRules == WRAP_CONTENT ) {
				setInternalPixelsWidth( mSprite->getCurrentTextureRegion()->getPxSize().getWidth() + mRealPadding.Left + mRealPadding.Right );
			}

			if ( mLayoutHeightRules == WRAP_CONTENT ) {
				setInternalPixelsHeight( mSprite->getCurrentTextureRegion()->getPxSize().getHeight() + mRealPadding.Top + mRealPadding.Bottom );
			}
		}
	}
}

void UISprite::autoAlign() {
	if ( NULL == mSprite || NULL == mSprite->getCurrentTextureRegion() )
		return;

	TextureRegion * tTextureRegion = mSprite->getCurrentTextureRegion();

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = ( mSize.getWidth() - tTextureRegion->getPxSize().getWidth() ) / 2;
	} else if ( fontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.getWidth() - tTextureRegion->getPxSize().getWidth() - mRealPadding.Right;
	} else {
		mAlignOffset.x = mRealPadding.Left;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = ( mSize.getHeight() - tTextureRegion->getPxSize().getHeight() ) / 2;
	} else if ( fontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - tTextureRegion->getPxSize().getHeight() - mRealPadding.Bottom;
	} else {
		mAlignOffset.y = mRealPadding.Top;
	}
}

const Vector2f& UISprite::getAlignOffset() const {
	return mAlignOffset;
}

void UISprite::setIsSpriteOwner( const bool& dealloc ) {
	writeNodeFlag( NODE_FLAG_FREE_USE, dealloc ? 1 : 0 );
}

bool UISprite::getDeallocSprite() {
	return 0 != ( mNodeFlags & NODE_FLAG_FREE_USE );
}

void UISprite::onSizeChange() {
	autoAlign();
	notifyLayoutAttrChange();
	UIWidget::onSizeChange();
}

bool UISprite::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) ) return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Src:
		{
			std::string val = attribute.asString();

			if ( val.size() ) {
				setIsSpriteOwner( true );
				setSprite( Sprite::New( val ) );
			}
			break;
		}
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

}}
