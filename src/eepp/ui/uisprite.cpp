#include <eepp/ui/uisprite.hpp>
#include <eepp/graphics/sprite.hpp>

namespace EE { namespace UI {

UISprite::UISprite( const UISprite::CreateParams& Params ) :
	UIComplexControl( Params ),
	mSprite( Params.Sprite ),
	mRender( Params.SpriteRender ),
	mAlignOffset(0,0),
	mSubTextureLast(NULL)
{
	if ( Params.DealloSprite )
		mControlFlags |= UI_CTRL_FLAG_FREE_USE;

	if ( ( flags() & UI_AUTO_SIZE ) || ( Params.Size.x == -1 && Params.Size.y == -1 ) ) {
		if ( NULL != mSprite && NULL != mSprite->getCurrentSubTexture() ) {
			size( mSprite->getCurrentSubTexture()->size() );
		}
	}
}

UISprite::~UISprite() {
	if ( deallocSprite() )
		eeSAFE_DELETE( mSprite );
}

Uint32 UISprite::getType() const {
	return UI_TYPE_SPRITE;
}

bool UISprite::isType( const Uint32& type ) const {
	return UISprite::getType() == type ? true : UIComplexControl::isType( type );
}

Uint32 UISprite::deallocSprite() {
	return mControlFlags & UI_CTRL_FLAG_FREE_USE;
}

void UISprite::sprite( Graphics::Sprite * sprite ) {
	if ( deallocSprite() )
		eeSAFE_DELETE( mSprite );

	mSprite = sprite;
	
	updateSize();
}

void UISprite::draw() {
	UIControlAnim::draw();

	if ( mVisible ) {
		if ( NULL != mSprite && 0.f != mAlpha ) {
			checkSubTextureUpdate();
			mSprite->position( (Float)( mScreenPos.x + mAlignOffset.x ), (Float)( mScreenPos.y + mAlignOffset.y ) );
			mSprite->draw( blend(), mRender );
		}
	}
}

void UISprite::checkSubTextureUpdate() {
	if ( NULL != mSprite && NULL != mSprite->getCurrentSubTexture() && mSprite->getCurrentSubTexture() != mSubTextureLast ) {
		updateSize();
		autoAlign();
		mSubTextureLast = mSprite->getCurrentSubTexture();
	}
}

void UISprite::alpha( const Float& alpha ) {
	UIControlAnim::alpha( alpha );
	
	if ( NULL != mSprite )
		mSprite->alpha( alpha );
}

Graphics::Sprite * UISprite::sprite() const {
	return mSprite;
}

ColorA UISprite::color() const {
	if ( NULL != mSprite )
		return mSprite->color();

	return ColorA();
}

void UISprite::color( const ColorA& color ) {
	if ( NULL != mSprite )
		mSprite->color( color );
	
	alpha( color.a() );
}

const EE_RENDER_MODE& UISprite::renderMode() const {
	return mRender;
}

void UISprite::renderMode( const EE_RENDER_MODE& render ) {
	mRender = render;
}

void UISprite::updateSize() {
	if ( flags() & UI_AUTO_SIZE ) {
		if ( NULL != mSprite ) {
			if ( NULL != mSprite->getCurrentSubTexture() && mSprite->getCurrentSubTexture()->size() != mSize )
				size( mSprite->getCurrentSubTexture()->size() );
		}
	}
}

void UISprite::autoAlign() {
	if ( NULL == mSprite || NULL == mSprite->getCurrentSubTexture() )
		return;

	SubTexture * tSubTexture = mSprite->getCurrentSubTexture();

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.width() / 2 - tSubTexture->size().width() / 2;
	} else if ( FontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.width() - tSubTexture->size().width();
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.height() / 2 - tSubTexture->size().height() / 2;
	} else if ( FontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.height() - tSubTexture->size().height();
	} else {
		mAlignOffset.y = 0;
	}
}

const Vector2i& UISprite::alignOffset() const {
	return mAlignOffset;
}

void UISprite::onSizeChange() {
	autoAlign();
}

}}
