#include <eepp/ui/uisprite.hpp>
#include <eepp/graphics/sprite.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>

namespace EE { namespace UI {

UISprite * UISprite::New() {
	return eeNew( UISprite, () );
}

UISprite::UISprite() :
	UIWidget(),
	mSprite( NULL ),
	mRender( RENDER_NORMAL ),
	mAlignOffset(0,0),
	mTextureRegionLast(NULL)
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

void UISprite::update( const Time& time ) {
	UIWidget::update( time );

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
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mSprite ) {
			if ( NULL != mSprite->getCurrentTextureRegion() && mSprite->getCurrentTextureRegion()->getDpSize().asFloat() != mSize )
				setSize( mSprite->getCurrentTextureRegion()->getDpSize().asFloat() );
		}
	}
}

void UISprite::autoAlign() {
	if ( NULL == mSprite || NULL == mSprite->getCurrentTextureRegion() )
		return;

	TextureRegion * tTextureRegion = mSprite->getCurrentTextureRegion();

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = mSize.getWidth() / 2 - tTextureRegion->getDpSize().getWidth() / 2;
	} else if ( fontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.getWidth() - tTextureRegion->getDpSize().getWidth();
	} else {
		mAlignOffset.x = 0;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = mSize.getHeight() / 2 - tTextureRegion->getDpSize().getHeight() / 2;
	} else if ( fontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - tTextureRegion->getDpSize().getHeight();
	} else {
		mAlignOffset.y = 0;
	}
}

const Vector2f& UISprite::getAlignOffset() const {
	return mAlignOffset;
}

void UISprite::setDeallocSprite( const bool& dealloc ) {
	writeCtrlFlag( NODE_FLAG_FREE_USE, dealloc ? 1 : 0 );
}

bool UISprite::getDeallocSprite() {
	return 0 != ( mNodeFlags & NODE_FLAG_FREE_USE );
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
