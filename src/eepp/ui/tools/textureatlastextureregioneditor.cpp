#include <eepp/graphics/primitives.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/ui/tools/textureatlastextureregioneditor.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI { namespace Tools {

TextureAtlasTextureRegionEditor*
TextureAtlasTextureRegionEditor::TextureAtlasTextureRegionEditor::New(
	TextureAtlasEditor* Editor ) {
	return eeNew( TextureAtlasTextureRegionEditor, ( Editor ) );
}

TextureAtlasTextureRegionEditor::TextureAtlasTextureRegionEditor( TextureAtlasEditor* Editor ) :
	UIWidget(), mGfx( NULL ), mEditor( Editor ) {
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
	mDragPos = mDrag->getPixelsPosition();

	mDrag->addEventListener( Event::OnPositionChange, [this]( const Event* event ) {
		if ( NULL != mGfx->getTextureRegion() ) {
			Vector2f Diff = -( mDragPos - mDrag->getPixelsPosition() );

			mDragPos = mDrag->getPixelsPosition();

			Diff = PixelDensity::pxToDp( Diff );

			mGfx->getTextureRegion()->setOffset(
				Vector2i( mGfx->getTextureRegion()->getOffset().x + Diff.x,
						  mGfx->getTextureRegion()->getOffset().y + Diff.y ) );

			mEditor->getSpinOffX()->setValue( mGfx->getTextureRegion()->getOffset().x );
			mEditor->getSpinOffY()->setValue( mGfx->getTextureRegion()->getOffset().y );
		}

		mGfx->setPosition( mUICenter );
	} );

	getCenter();
	mGfx->setPosition( mUICenter );
}

TextureAtlasTextureRegionEditor::~TextureAtlasTextureRegionEditor() {}

void TextureAtlasTextureRegionEditor::draw() {
	UIWidget::draw();

	Primitives P;
	P.setColor( Color( 255, 0, 0, mAlpha ) );
	P.setLineWidth( PixelDensity::dpToPx( 1.f ) );

	Vector2f uiCenterPx = PixelDensity::dpToPx( mUICenter );

	P.drawLine(
		Line2f( Vector2f( mScreenPos.x, mScreenPos.y + uiCenterPx.y ),
				Vector2f( mScreenPos.x + mSize.getWidth(), mScreenPos.y + uiCenterPx.y ) ) );
	P.drawLine(
		Line2f( Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y ),
				Vector2f( mScreenPos.x + uiCenterPx.x, mScreenPos.y + mSize.getHeight() ) ) );
}

void TextureAtlasTextureRegionEditor::onSizeChange() {
	getCenter();
	mGfx->setPosition( mUICenter );
}

Graphics::TextureRegion* TextureAtlasTextureRegionEditor::getTextureRegion() const {
	return mGfx->getTextureRegion();
}

void TextureAtlasTextureRegionEditor::setTextureRegion( Graphics::TextureRegion* TextureRegion ) {
	mGfx->setTextureRegion( TextureRegion );
}

UITextureRegion* TextureAtlasTextureRegionEditor::getGfx() const {
	return mGfx;
}

void TextureAtlasTextureRegionEditor::getCenter() {
	mUICenter = Vector2f( getSize().getWidth() / 2, getSize().getHeight() / 2 );
}

}}} // namespace EE::UI::Tools
