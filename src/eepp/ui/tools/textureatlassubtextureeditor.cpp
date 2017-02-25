#include <eepp/ui/tools/textureatlassubtextureeditor.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI { namespace Tools {

TextureAtlasSubTextureEditor::TextureAtlasSubTextureEditor( const UIComplexControl::CreateParams& Params, TextureAtlasEditor * Editor ) :
	UIComplexControl( Params ),
	mGfx( NULL ),
	mEditor( Editor )
{
	if ( NULL == UIThemeManager::instance()->getDefaultTheme() ) {
		return;
	}

	mTheme = UIThemeManager::instance()->getDefaultTheme();

	mGfx = eeNew( UIGfx, () );
	mGfx->setParent( this );
	mGfx->setVisible( true );
	mGfx->setEnabled( true );
	mGfx->unsetFlags( UI_FIT_TO_CONTROL );

	UIDragable::CreateParams DragParams;
	DragParams.setParent( this );
	DragParams.setSize( 500000, 500000 );
	mDrag = eeNew( UIDragable, ( DragParams ) );
	mDrag->setEnabled( true );
	mDrag->setVisible( true );
	mDrag->setDragEnabled( true );
	mDrag->center();

	getCenter();
}

TextureAtlasSubTextureEditor::~TextureAtlasSubTextureEditor() {
}

void TextureAtlasSubTextureEditor::draw() {
	Primitives P;
	P.setColor( ColorA( 255, 0, 0, mAlpha ) );
	P.setLineWidth( PixelDensity::dpToPx( 1.f ) );

	Vector2i uiCenterPx = PixelDensity::dpToPxI( mUICenter );

	P.drawLine( Line2f( Vector2f( mScreenPos.x, mScreenPos.y + uiCenterPx.y ), Vector2f( mScreenPos.x + mRealSize.getWidth(), mScreenPos.y + uiCenterPx.y ) ) );
	P.drawLine( Line2f( Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y ), Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y + mRealSize.getHeight() ) ) );

	UIComplexControl::draw();
}

void TextureAtlasSubTextureEditor::update() {
	Vector2i Pos = mDrag->getRealPosition();

	UIComplexControl::update();

	if ( NULL != mGfx->getSubTexture() && mDrag->isDragEnabled() && mDrag->isDragging() && Pos != mDrag->getRealPosition() ) {
		Vector2i Diff = -( Pos - mDrag->getRealPosition() );

		Diff = PixelDensity::pxToDpI( Diff );

		mGfx->getSubTexture()->setOffset( Vector2i( mGfx->getSubTexture()->getOffset().x + Diff.x, mGfx->getSubTexture()->getOffset().y + Diff.y ) );

		mEditor->getSpinOffX()->setValue( mGfx->getSubTexture()->getOffset().x );
		mEditor->getSpinOffY()->setValue( mGfx->getSubTexture()->getOffset().y );
	}

	mGfx->setPosition( mUICenter );
}

void TextureAtlasSubTextureEditor::onSizeChange() {
	getCenter();
}

Graphics::SubTexture * TextureAtlasSubTextureEditor::getSubTexture() const {
	return mGfx->getSubTexture();
}

void TextureAtlasSubTextureEditor::setSubTexture( Graphics::SubTexture * subTexture ) {
	mGfx->setSubTexture( subTexture );
}

UIGfx * TextureAtlasSubTextureEditor::getGfx() const {
	return mGfx;
}

void TextureAtlasSubTextureEditor::getCenter() {
	mUICenter = Vector2i( mSize.getWidth() / 2, mSize.getHeight() / 2 );
}

}}}
