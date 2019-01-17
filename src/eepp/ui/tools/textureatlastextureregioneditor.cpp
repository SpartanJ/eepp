#include <eepp/ui/tools/textureatlastextureregioneditor.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

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

	subscribeScheduledUpdate();

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

	Vector2f uiCenterPx = PixelDensity::dpToPx( mUICenter );

	P.drawLine( Line2f( Vector2f( mScreenPos.x, mScreenPos.y + uiCenterPx.y ), Vector2f( mScreenPos.x + mSize.getWidth(), mScreenPos.y + uiCenterPx.y ) ) );
	P.drawLine( Line2f( Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y ), Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y + mSize.getHeight() ) ) );
}

void TextureAtlasTextureRegionEditor::scheduledUpdate( const Time& ) {
	Vector2f Pos = mDrag->getPixelsPosition();

	if ( NULL != mGfx->getTextureRegion() && mDrag->isDragEnabled() && mDrag->isDragging() && Pos != mDrag->getPixelsPosition() ) {
		Vector2f Diff = -( Pos - mDrag->getPixelsPosition() );

		Diff = PixelDensity::pxToDp( Diff );

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
	mUICenter = Vector2f( mDpSize.getWidth() / 2, mDpSize.getHeight() / 2 );
}

}}}
