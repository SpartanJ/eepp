#include <eepp/ui/tools/textureatlastextureregioneditor.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI { namespace Tools {

TextureAtlasTextureRegionEditor * TextureAtlasTextureRegionEditor::TextureAtlasTextureRegionEditor::New( TextureAtlasEditor * Editor ) {
	return eeNew( TextureAtlasTextureRegionEditor, ( Editor ) );
}

TextureAtlasTextureRegionEditor::TextureAtlasTextureRegionEditor( TextureAtlasEditor * Editor ) :
	UIWidget(),
	mGfx( NULL ),
	mEditor( Editor )
{
	if ( NULL == UIThemeManager::instance()->getDefaultTheme() ) {
		return;
	}

	mTheme = UIThemeManager::instance()->getDefaultTheme();

	mGfx = UITextureRegion::New();
	mGfx->setParent( this );
	mGfx->setVisible( true );
	mGfx->setEnabled( true );

	mDrag = UINode::New();
	mDrag->setParent( this );
	mDrag->setSize( 64000, 64000 );
	mDrag->setEnabled( true );
	mDrag->setVisible( true );
	mDrag->setDragEnabled( true );
	mDrag->center();

	getCenter();
}

TextureAtlasTextureRegionEditor::~TextureAtlasTextureRegionEditor() {
}

void TextureAtlasTextureRegionEditor::draw() {
	UIWidget::draw();

	Primitives P;
	P.setColor( Color( 255, 0, 0, mAlpha ) );
	P.setLineWidth( PixelDensity::dpToPx( 1.f ) );

	Vector2i uiCenterPx = PixelDensity::dpToPxI( mUICenter );

	P.drawLine( Line2f( Vector2f( mScreenPos.x, mScreenPos.y + uiCenterPx.y ), Vector2f( mScreenPos.x + mRealSize.getWidth(), mScreenPos.y + uiCenterPx.y ) ) );
	P.drawLine( Line2f( Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y ), Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y + mRealSize.getHeight() ) ) );
}

void TextureAtlasTextureRegionEditor::update() {
	Vector2i Pos = mDrag->getRealPosition();

	UIWidget::update();

	if ( NULL != mGfx->getTextureRegion() && mDrag->isDragEnabled() && mDrag->isDragging() && Pos != mDrag->getRealPosition() ) {
		Vector2i Diff = -( Pos - mDrag->getRealPosition() );

		Diff = PixelDensity::pxToDpI( Diff );

		mGfx->getTextureRegion()->setOffset( Vector2i( mGfx->getTextureRegion()->getOffset().x + Diff.x, mGfx->getTextureRegion()->getOffset().y + Diff.y ) );

		mEditor->getSpinOffX()->setValue( mGfx->getTextureRegion()->getOffset().x );
		mEditor->getSpinOffY()->setValue( mGfx->getTextureRegion()->getOffset().y );
	}

	mGfx->setPosition( mUICenter );
}

void TextureAtlasTextureRegionEditor::onSizeChange() {
	getCenter();
}

Graphics::TextureRegion * TextureAtlasTextureRegionEditor::getTextureRegion() const {
	return mGfx->getTextureRegion();
}

void TextureAtlasTextureRegionEditor::setTextureRegion( Graphics::TextureRegion * TextureRegion ) {
	mGfx->setTextureRegion( TextureRegion );
}

UITextureRegion * TextureAtlasTextureRegionEditor::getGfx() const {
	return mGfx;
}

void TextureAtlasTextureRegionEditor::getCenter() {
	mUICenter = Vector2i( mSize.getWidth() / 2, mSize.getHeight() / 2 );
}

}}}
