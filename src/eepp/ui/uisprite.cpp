#include <eepp/ui/uisprite.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>

namespace EE { namespace UI {

UISprite * UISprite::New() {
	return eeNew( UISprite, () );
}

UISprite::UISprite() :
	UIWidget(),
	mSprite( NULL ),
	mRender( RN_NORMAL ),
	mAlignOffset(0,0),
	mSubTextureLast(NULL)
{
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
	return mControlFlags & UI_CTRL_FLAG_FREE_USE;
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
			checkSubTextureUpdate();

			mSprite->setPosition( Vector2f( (Float)( mScreenPos.x + mAlignOffset.x ), (Float)( mScreenPos.y + mAlignOffset.y ) ) );

			SubTexture * subTexture = mSprite->getCurrentSubTexture();

			if ( NULL != subTexture ) {
				Sizef oDestSize = subTexture->getDestSize();
				Sizei pxSize = subTexture->getPxSize();

				subTexture->setDestSize( Sizef( (Float)pxSize.x, (Float)pxSize.y ) );

				mSprite->draw( getBlendMode(), mRender );

				subTexture->setDestSize( oDestSize );
			}
		}
	}
}

void UISprite::update() {
	UIWidget::update();

	if ( NULL != mSprite ) {
		SubTexture * subTexture = mSprite->getCurrentSubTexture();

		mSprite->update();

		if ( subTexture != mSprite->getCurrentSubTexture() )
			invalidateDraw();
	}
}

void UISprite::checkSubTextureUpdate() {
	if ( NULL != mSprite && NULL != mSprite->getCurrentSubTexture() && mSprite->getCurrentSubTexture() != mSubTextureLast ) {
		updateSize();
		autoAlign();
		mSubTextureLast = mSprite->getCurrentSubTexture();
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

const EE_RENDER_MODE& UISprite::getRenderMode() const {
	return mRender;
}

void UISprite::setRenderMode( const EE_RENDER_MODE& render ) {
	mRender = render;
	invalidateDraw();
}

void UISprite::updateSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mSprite ) {
			if ( NULL != mSprite->getCurrentSubTexture() && mSprite->getCurrentSubTexture()->getDpSize() != mSize )
				setSize( mSprite->getCurrentSubTexture()->getDpSize() );
		}
	}
}

void UISprite::autoAlign() {
	if ( NULL == mSprite || NULL == mSprite->getCurrentSubTexture() )
		return;

	SubTexture * tSubTexture = mSprite->getCurrentSubTexture();

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.getWidth() / 2 - tSubTexture->getDpSize().getWidth() / 2;
	} else if ( fontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.getWidth() - tSubTexture->getDpSize().getWidth();
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.getHeight() / 2 - tSubTexture->getDpSize().getHeight() / 2;
	} else if ( fontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - tSubTexture->getDpSize().getHeight();
	} else {
		mAlignOffset.y = 0;
	}
}

const Vector2i& UISprite::getAlignOffset() const {
	return mAlignOffset;
}

void UISprite::setDeallocSprite( const bool& dealloc ) {
	writeCtrlFlag( UI_CTRL_FLAG_FREE_USE, dealloc ? 1 : 0 );
}

bool UISprite::getDeallocSprite() {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_FREE_USE );
}

void UISprite::onSizeChange() {
	autoAlign();
	notifyLayoutAttrChange();
	UIWidget::onSizeChange();
}

void UISprite::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "src" == name ) {
			std::string val = ait->as_string();

			if ( val.size() ) {
				setDeallocSprite( true );
				setSprite( eeNew( Sprite, ( val ) ) );
			}
		}
	}

	endPropertiesTransaction();
}

}}
